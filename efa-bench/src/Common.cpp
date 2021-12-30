#include "Common.h"

bool common::is_benchmark(std::string t1, T t2)
{
    return t1.compare(BENCHMARK_TYPE[t2]) == 0;
}

bool common::is_node(std::string t1, T t2)
{
    return t1.compare(NODE_TYPE[t2]) == 0;
}

void common::setBaseFabricHints(fi_info *hints)
{
    hints->mode |= (FLAGS_endpoint == "dgram") ? FI_MSG_PREFIX : FI_CONTEXT;
    hints->caps = FLAGS_tagged ? FI_TAGGED : FI_MSG;
    hints->domain_attr->resource_mgmt = (FLAGS_endpoint == "dgram") ? FI_RM_DISABLED : FI_RM_ENABLED;
    hints->domain_attr->threading = FI_THREAD_DOMAIN;
    hints->tx_attr->tclass = FI_TC_LOW_LATENCY;
}

void common::setRmaFabricHints(fi_info *hints)
{
	hints->mode |= FI_CONTEXT;
	hints->caps = FI_MSG | FI_RMA;
	hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
	hints->domain_attr->threading = FI_THREAD_DOMAIN;
	hints->tx_attr->tclass = FI_TC_BULK_DATA;
}