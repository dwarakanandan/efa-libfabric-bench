#include "BenchmarkServer.h"

using namespace std;
using namespace libefa;

void startPingPongServer()
{
    int ret;
    fi_info *hints = fi_allocinfo();

    if (!hints)
        return;

    hints->caps = FI_TAGGED;
    hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    hints->domain_attr->threading = FI_THREAD_DOMAIN;
    hints->tx_attr->tclass = FI_TC_LOW_LATENCY;

    Server server = Server(FLAGS_provider, FLAGS_endpoint, hints);
    server.init();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    server.startTimer();
    for (int i = 0; i < FLAGS_iterations; i++)
    {
        ret = server.rx();
        if (ret)
            return;
        ret = server.tx();
        if (ret)
            return;
    }
    server.stopTimer();

    server.showTransferStatistics(FLAGS_iterations, 2);
}

void startPingPongInjectServer()
{
}

void startTaggedBatchServer()
{
}

void startLatencyTestServer()
{
}

void startCapsTestServer()
{
}

void startRmaServer()
{
    int ret;

    hints = fi_allocinfo();
    if (!hints)
        return ;

    hints->caps = FI_MSG | FI_RMA;
    hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    hints->domain_attr->threading = FI_THREAD_DOMAIN;
    hints->tx_attr->tclass = FI_TC_BULK_DATA;

    Server server = Server(FLAGS_provider, FLAGS_endpoint, hints);
    server.initRmaOp("write");

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    server.startTimer();
    for (int i = 0; i < FLAGS_iterations; i++)
    {
        ret = server.rma();
        if (ret)
            return;
    }
    server.stopTimer();

    // Pingpong once after RMA ops are complete
    server.rx();
    server.tx();

    server.showTransferStatistics(FLAGS_iterations, 1);
}