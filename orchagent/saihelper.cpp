extern "C" {
#include "sai.h"
#include "saistatus.h"
}

#include <map>
#include <logger.h>
#include "saihelper.h"

using namespace std;
using namespace swss;

/* Initialize all global api pointers */
sai_switch_api_t*           sai_switch_api;
sai_virtual_router_api_t*   sai_virtual_router_api;
sai_port_api_t*             sai_port_api;
sai_vlan_api_t*             sai_vlan_api;
sai_router_interface_api_t* sai_router_intfs_api;
sai_hostif_api_t*           sai_hostif_api;
sai_neighbor_api_t*         sai_neighbor_api;
sai_next_hop_api_t*         sai_next_hop_api;
sai_next_hop_group_api_t*   sai_next_hop_group_api;
sai_route_api_t*            sai_route_api;
sai_lag_api_t*              sai_lag_api;
sai_policer_api_t*          sai_policer_api;
sai_tunnel_api_t*           sai_tunnel_api;
sai_queue_api_t*            sai_queue_api;
sai_scheduler_api_t*        sai_scheduler_api;
sai_scheduler_group_api_t*  sai_scheduler_group_api;
sai_wred_api_t*             sai_wred_api;
sai_qos_map_api_t*          sai_qos_map_api;
sai_buffer_api_t*           sai_buffer_api;
sai_acl_api_t*              sai_acl_api;
sai_mirror_api_t*           sai_mirror_api;
sai_fdb_api_t*              sai_fdb_api;

map<string, string> gProfileMap;

const char *test_profile_get_value (
    _In_ sai_switch_profile_id_t profile_id,
    _In_ const char *variable)
{
    SWSS_LOG_ENTER();

    auto it = gProfileMap.find(variable);

    if (it == gProfileMap.end())
        return NULL;
    return it->second.c_str();
}

int test_profile_get_next_value (
    _In_ sai_switch_profile_id_t profile_id,
    _Out_ const char **variable,
    _Out_ const char **value)
{
    SWSS_LOG_ENTER();

    static auto it = gProfileMap.begin();

    if (value == NULL)
    {
        // Restarts enumeration
        it = gProfileMap.begin();
    }
    else if (it == gProfileMap.end())
    {
        return -1;
    }
    else
    {
        *variable = it->first.c_str();
        *value = it->second.c_str();
        it++;
    }

    if (it != gProfileMap.end())
        return 0;
    else
        return -1;
}

const service_method_table_t test_services = {
    test_profile_get_value,
    test_profile_get_next_value
};

void initSaiApi()
{
    SWSS_LOG_ENTER();

    sai_api_initialize(0, (service_method_table_t *)&test_services);

    sai_api_query(SAI_API_SWITCH,               (void **)&sai_switch_api);
    sai_api_query(SAI_API_VIRTUAL_ROUTER,       (void **)&sai_virtual_router_api);
    sai_api_query(SAI_API_PORT,                 (void **)&sai_port_api);
    sai_api_query(SAI_API_FDB,                  (void **)&sai_fdb_api);
    sai_api_query(SAI_API_VLAN,                 (void **)&sai_vlan_api);
    sai_api_query(SAI_API_HOST_INTERFACE,       (void **)&sai_hostif_api);
    sai_api_query(SAI_API_MIRROR,               (void **)&sai_mirror_api);
    sai_api_query(SAI_API_ROUTER_INTERFACE,     (void **)&sai_router_intfs_api);
    sai_api_query(SAI_API_NEIGHBOR,             (void **)&sai_neighbor_api);
    sai_api_query(SAI_API_NEXT_HOP,             (void **)&sai_next_hop_api);
    sai_api_query(SAI_API_NEXT_HOP_GROUP,       (void **)&sai_next_hop_group_api);
    sai_api_query(SAI_API_ROUTE,                (void **)&sai_route_api);
    sai_api_query(SAI_API_LAG,                  (void **)&sai_lag_api);
    sai_api_query(SAI_API_POLICER,              (void **)&sai_policer_api);
    sai_api_query(SAI_API_TUNNEL,               (void **)&sai_tunnel_api);
    sai_api_query(SAI_API_QUEUE,                (void **)&sai_queue_api);
    sai_api_query(SAI_API_SCHEDULER,            (void **)&sai_scheduler_api);
    sai_api_query(SAI_API_WRED,                 (void **)&sai_wred_api);
    sai_api_query(SAI_API_QOS_MAPS,             (void **)&sai_qos_map_api);
    sai_api_query(SAI_API_BUFFERS,              (void **)&sai_buffer_api);
    sai_api_query(SAI_API_SCHEDULER_GROUP,      (void **)&sai_scheduler_group_api);
    sai_api_query(SAI_API_ACL,                  (void **)&sai_acl_api);

    sai_log_set(SAI_API_SWITCH,                 SAI_LOG_NOTICE);
    sai_log_set(SAI_API_VIRTUAL_ROUTER,         SAI_LOG_NOTICE);
    sai_log_set(SAI_API_PORT,                   SAI_LOG_NOTICE);
    sai_log_set(SAI_API_FDB,                    SAI_LOG_NOTICE);
    sai_log_set(SAI_API_VLAN,                   SAI_LOG_NOTICE);
    sai_log_set(SAI_API_HOST_INTERFACE,         SAI_LOG_NOTICE);
    sai_log_set(SAI_API_MIRROR,                 SAI_LOG_NOTICE);
    sai_log_set(SAI_API_ROUTER_INTERFACE,       SAI_LOG_NOTICE);
    sai_log_set(SAI_API_NEIGHBOR,               SAI_LOG_NOTICE);
    sai_log_set(SAI_API_NEXT_HOP,               SAI_LOG_NOTICE);
    sai_log_set(SAI_API_NEXT_HOP_GROUP,         SAI_LOG_NOTICE);
    sai_log_set(SAI_API_ROUTE,                  SAI_LOG_NOTICE);
    sai_log_set(SAI_API_LAG,                    SAI_LOG_NOTICE);
    sai_log_set(SAI_API_POLICER,                SAI_LOG_NOTICE);
    sai_log_set(SAI_API_TUNNEL,                 SAI_LOG_NOTICE);
    sai_log_set(SAI_API_QUEUE,                  SAI_LOG_NOTICE);
    sai_log_set(SAI_API_SCHEDULER,              SAI_LOG_NOTICE);
    sai_log_set(SAI_API_WRED,                   SAI_LOG_NOTICE);
    sai_log_set(SAI_API_QOS_MAPS,               SAI_LOG_NOTICE);
    sai_log_set(SAI_API_BUFFERS,                SAI_LOG_NOTICE);
    sai_log_set(SAI_API_SCHEDULER_GROUP,        SAI_LOG_NOTICE);
    sai_log_set(SAI_API_ACL,                    SAI_LOG_NOTICE);
}
