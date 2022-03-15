#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>
#include <signal.h>
#include "logger.h"
#include "select.h"
#include "netdispatcher.h"
#include "natsync.h"
#include <netlink/netfilter/nfnl.h>

#if defined(ASAN_ENABLED)
#include <sanitizer/lsan_interface.h>
#endif

using namespace std;
using namespace swss;

#if defined(ASAN_ENABLED)
void sigterm_handler(int signo)
{
    __lsan_do_leak_check();
    signal(signo, SIG_DFL);
    raise(signo);
}
#endif

void *alloc()
{
    return malloc(1000);
}

void use_ptr(void *ptr)
{
    memset(ptr, 0, 1000);
}

int main(int argc, char **argv)
{
    Logger::linkToDbNative("natsyncd");

#if defined(ASAN_ENABLED)
    if (signal(SIGTERM, sigterm_handler) == SIG_ERR)
    {
        SWSS_LOG_ERROR("failed to setup SIGTERM action");
        exit(1);
    }
#endif

    use_ptr(alloc());

    DBConnector     appDb("APPL_DB", 0);
    DBConnector     stateDb("STATE_DB", 0);
    RedisPipeline   pipelineAppDB(&appDb);
    NfNetlink       nfnl;

    nfnl.registerRecvCallbacks();
    NatSync sync(&pipelineAppDB, &appDb, &stateDb, &nfnl);

    sync.isPortInitDone(&appDb);

    NetDispatcher::getInstance().registerMessageHandler(NFNLMSG_TYPE(NFNL_SUBSYS_CTNETLINK, IPCTNL_MSG_CT_NEW), &sync);
    NetDispatcher::getInstance().registerMessageHandler(NFNLMSG_TYPE(NFNL_SUBSYS_CTNETLINK, IPCTNL_MSG_CT_DELETE), &sync);

    while (1)
    {
        try
        {
            Select s;

            using namespace std::chrono;
            /*
             * If warmstart, read the NAT tables to cache map.
             * Wait for the kernel NAT conntrack table restore to finish in case of warmreboot.
             * Start reconcile timer once restore flag is set.
             */
            if (sync.getRestartAssist()->isWarmStartInProgress())
            {
                sync.getRestartAssist()->readTablesToMap();

                steady_clock::time_point starttime = steady_clock::now();
                while (!sync.isNatRestoreDone())
                {
                    duration<double> time_span =
                        duration_cast<duration<double>>(steady_clock::now() - starttime);
                    int pasttime = int(time_span.count());
                    SWSS_LOG_INFO("Waited for NAT conntrack table to be restored to kernel"
                      " for %d seconds", pasttime);
                    if (pasttime > RESTORE_NAT_WAIT_TIME_OUT)
                    {
                        SWSS_LOG_ERROR("Nat conntrack table restore is not finished"
                            " after timed-out, exit!!!");
                        exit(EXIT_FAILURE);
                    }
                    sleep(1);
                }
                sync.getRestartAssist()->startReconcileTimer(s);
            }

            nfnl.registerGroup(NFNLGRP_CONNTRACK_NEW);
            nfnl.registerGroup(NFNLGRP_CONNTRACK_UPDATE);
            nfnl.registerGroup(NFNLGRP_CONNTRACK_DESTROY);

            SWSS_LOG_INFO("Listens to conntrack messages...");
            nfnl.dumpRequest(IPCTNL_MSG_CT_GET);

            s.addSelectable(&nfnl);
            while (true)
            {
                Selectable *temps;
                s.select(&temps);
                /*
                 * If warmstart is in progress, we check the reconcile timer,
                 * if timer expired, we stop the timer and start the reconcile process
                 */
                if (sync.getRestartAssist()->isWarmStartInProgress())
                {
                    if (sync.getRestartAssist()->checkReconcileTimer(temps))
                    {
                        sync.getRestartAssist()->stopReconcileTimer(s);
                        sync.getRestartAssist()->reconcile();
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            SWSS_LOG_ERROR("Runtime error: %s", e.what());
            return 0;
        }
    }

    return 1;
}
