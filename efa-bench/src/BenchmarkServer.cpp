#include "BenchmarkServer.h"

using namespace std;
using namespace libefa;

void startPingPongServer()
{
    int ret;
    fi_info *hints = fi_allocinfo();
    hints->mode |= (FLAGS_endpoint == "dgram") ? FI_MSG_PREFIX : FI_CONTEXT;
    hints->caps = FLAGS_tagged ? FI_TAGGED : FI_MSG;
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
    int ret;
    fi_info *hints = fi_allocinfo();
    hints->mode |= (FLAGS_endpoint == "dgram") ? FI_MSG_PREFIX : FI_CONTEXT;
    hints->caps = FLAGS_tagged ? FI_TAGGED : FI_MSG;
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
        ret = server.inject();
        if (ret)
            return;
    }
    server.stopTimer();

    server.showTransferStatistics(FLAGS_iterations, 2);
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

    fi_info *hints = fi_allocinfo();
    hints->mode |= FI_CONTEXT;
    hints->caps = FI_MSG | FI_RMA;
    hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    hints->domain_attr->threading = FI_THREAD_DOMAIN;
    hints->tx_attr->tclass = FI_TC_BULK_DATA;

    Server server = Server(FLAGS_provider, FLAGS_endpoint, hints);
    server.initRmaOp(FLAGS_rma_op);

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

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(FLAGS_iterations, 1);
}