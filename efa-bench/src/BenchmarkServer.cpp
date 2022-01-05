#include "BenchmarkServer.h"

using namespace std;
using namespace libefa;

void startPingPongServer()
{
    int ret;
    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

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
    common::setBaseFabricHints(hints);

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
    int ret;
    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, hints);
    server.init();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    server.startTimer();
    int numTxRetries = 0;
    int numCqObtained = 0;
    int cqTry = FLAGS_batch * FLAGS_cq_try;

    for (int i = 1; i <= FLAGS_iterations; i++)
    {
        ret = server.postTx();
        while (ret == -FI_EAGAIN)
        {
            if (numCqObtained < i)
            {
                ret = server.getNTxCompletion(1);
                if (ret)
                {
                    printf("SERVER: getCqCompletion failed\n\n");
                    exit(1);
                }
                numCqObtained++;
            }

            // printf("fi_tsend retry iteration %d\n", i);
            ret = server.postTx();
            numTxRetries++;
        }
        if ((i - numCqObtained) > FLAGS_batch)
        {
            ret = server.getNTxCompletion(cqTry);
            if (ret)
            {
                printf("SERVER: getCqCompletion failed\n\n");
                exit(1);
            }
            numCqObtained += cqTry;
        }
    }

    ret = server.getNTxCompletion(FLAGS_iterations - numCqObtained);
    if (ret)
    {
        printf("SERVER: getCqCompletion failed\n\n");
        exit(1);
    }

    server.stopTimer();

    server.showTransferStatistics(FLAGS_iterations, 1);
}

void startLatencyTestServer()
{
    int ret;
    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, hints);
    server.init();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    server.startTimer();
    for (int i = 0; i < FLAGS_iterations; i++)
    {
        ret = server.tx();
        if (ret)
            return;
    }
    server.stopTimer();

    server.showTransferStatistics(FLAGS_iterations, 1);
}

void startCapsTestServer()
{
}

void startRmaServer()
{
    int ret;

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

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

void startRmaBatchServer()
{
    int ret;

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, hints);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    server.startTimer();
    int numTxRetries = 0;
    int numCqObtained = 0;
    int cqTry = FLAGS_batch * FLAGS_cq_try;

    for (int i = 1; i <= FLAGS_iterations; i++)
    {
        ret = server.postRma();
        while (ret == -FI_EAGAIN)
        {
            if (numCqObtained < i)
            {
                ret = server.getNTxCompletion(1);
                if (ret)
                {
                    printf("SERVER: getCqCompletion failed\n\n");
                    exit(1);
                }
                numCqObtained++;
            }

            // printf("fi_tsend retry iteration %d\n", i);
            ret = server.postRma();
            numTxRetries++;
        }
        if ((i - numCqObtained) > FLAGS_batch)
        {
            ret = server.getNTxCompletion(cqTry);
            if (ret)
            {
                printf("SERVER: getCqCompletion failed\n\n");
                exit(1);
            }
            numCqObtained += cqTry;
        }
    }

    ret = server.getNTxCompletion(FLAGS_iterations - numCqObtained);
    if (ret)
    {
        printf("SERVER: getCqCompletion failed\n\n");
        exit(1);
    }

    server.stopTimer();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(FLAGS_iterations, 1);
}

void startRmaInjectServer()
{
    int ret;

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, hints);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    server.startTimer();
    for (int i = 0; i < FLAGS_iterations; i++)
    {
        ret = server.postRmaInject();
        if (ret)
            return;
    }
    server.stopTimer();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(FLAGS_iterations, 1);
}

void startRmaSelectiveCompletionServer()
{
    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);
    //hints->tx_attr->op_flags = 0;

    Server server = Server(FLAGS_provider, FLAGS_endpoint, hints);
    server.enableSelectiveCompletion();
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    server.startTimer();
    int numPosted = 0;
    for (int i = 0; i < FLAGS_iterations; i++)
    {
        if (FLAGS_batch == numPosted)
        {
            numPosted = 0;
            server.postRmaSelectiveComp(true);
            server.getTxCompletion();
            continue;
        }
        server.postRmaSelectiveComp(false);
        numPosted++;
    }
    server.stopTimer();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(FLAGS_iterations, 1);
}