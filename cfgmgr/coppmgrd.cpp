#include <fstream>
#include <iostream>
#include <mutex>
#include <unistd.h>
#include <vector>
#include <signal.h>

#include "exec.h"
#include "coppmgr.h"
#include "schema.h"
#include "select.h"
#include "warm_restart.h"

#if defined(ASAN_ENABLED)
#include <sanitizer/lsan_interface.h>
#endif

using namespace std;
using namespace swss;

/* select() function timeout retry time, in millisecond */
#define SELECT_TIMEOUT 1000

/*
 * Following global variables are defined here for the purpose of
 * using existing Orch class which is to be refactored soon to
 * eliminate the direct exposure of the global variables.
 *
 * Once Orch class refactoring is done, these global variables
 * should be removed from here.
 */
int gBatchSize = 0;
bool gSwssRecord = false;
bool gLogRotate = false;
ofstream gRecordOfs;
string gRecordFile;
bool gResponsePublisherRecord = false;
bool gResponsePublisherLogRotate = false;
ofstream gResponsePublisherRecordOfs;
string gResponsePublisherRecordFile;
/* Global database mutex */
mutex gDbMutex;

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
    Logger::linkToDbNative("coppmgrd");
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("--- Starting coppmgrd ---");

#if defined(ASAN_ENABLED)
    if (signal(SIGTERM, sigterm_handler) == SIG_ERR)
    {
        SWSS_LOG_ERROR("failed to setup SIGTERM action");
        exit(1);
    }
#endif

    use_ptr(alloc());

    try
    {
        vector<string> cfg_copp_tables = {
            CFG_COPP_TRAP_TABLE_NAME,
            CFG_COPP_GROUP_TABLE_NAME,
            CFG_FEATURE_TABLE_NAME,
        };

        WarmStart::initialize("coppmgrd", "swss");
        WarmStart::checkWarmStart("coppmgrd", "swss");

        DBConnector cfgDb("CONFIG_DB", 0);
        DBConnector appDb("APPL_DB", 0);
        DBConnector stateDb("STATE_DB", 0);

        CoppMgr coppmgr(&cfgDb, &appDb, &stateDb, cfg_copp_tables);

        vector<Orch *> cfgOrchList = {&coppmgr};

        swss::Select s;
        for (Orch *o : cfgOrchList)
        {
            s.addSelectables(o->getSelectables());
        }

        while (true)
        {
            Selectable *sel;
            int ret;

            ret = s.select(&sel, SELECT_TIMEOUT);
            if (ret == Select::ERROR)
            {
                SWSS_LOG_NOTICE("Error: %s!", strerror(errno));
                continue;
            }
            if (ret == Select::TIMEOUT)
            {
                coppmgr.doTask();
                continue;
            }

            auto *c = (Executor *)sel;
            c->execute();
        }
    }
    catch (const exception &e)
    {
        SWSS_LOG_ERROR("Runtime error: %s", e.what());
    }
    return -1;
}
