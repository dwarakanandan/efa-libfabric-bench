#include "BenchmarkServer.h"

using namespace std;
using namespace libefa;

void pingPongServer(std::string port, std::string oobPort)
{
    int ret;
    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, port, oobPort, hints);
    server.init();
    server.sync();
    server.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    server.startTimer();
    while (true)
    {
        common::operationCounter += 2;
        ret = server.rx();
        if (ret)
            return;
        ret = server.tx();
        if (ret)
            return;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    server.stopTimer();
    server.showTransferStatistics(common::operationCounter / 2, 2);
}

void startPingPongServer()
{
    BenchmarkContext context;
    context.experimentName = FLAGS_benchmark_type;
    context.endpoint = FLAGS_endpoint;
    context.provider = FLAGS_provider;
    context.msgSize = FLAGS_payload;
    context.nodeType = FLAGS_mode;
    context.operationType = "send";
    context.batchSize = 1;
    context.numThreads = 1;

    CsvLogger logger = CsvLogger(context);

    logger.start();

    std::thread worker0(pingPongServer, "10000", "9000");
    std::thread worker1(pingPongServer, "10001", "9001");
    std::thread worker2(pingPongServer, "10002", "9002");
    std::thread worker3(pingPongServer, "10003", "9003");
    worker0.join();
    worker1.join();
    worker2.join();
    worker3.join();

    logger.stop();
}

void startPingPongInjectServer()
{
    int ret;
    BenchmarkContext context;
    context.experimentName = FLAGS_benchmark_type;
    context.endpoint = FLAGS_endpoint;
    context.provider = FLAGS_provider;
    context.msgSize = FLAGS_payload;
    context.nodeType = FLAGS_mode;
    context.operationType = "inject";
    context.batchSize = 1;
    context.numThreads = 1;

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
    server.init();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    logger.start();
    server.startTimer();
    while (true)
    {
        common::operationCounter += 2;
        ret = server.rx();
        if (ret)
            return;
        ret = server.inject();
        if (ret)
            return;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    server.stopTimer();
    logger.stop();

    server.showTransferStatistics(common::operationCounter / 2, 2);
}

void startTaggedBatchServer()
{
    int ret;
    BenchmarkContext context;
    context.experimentName = FLAGS_benchmark_type;
    context.endpoint = FLAGS_endpoint;
    context.provider = FLAGS_provider;
    context.msgSize = FLAGS_payload;
    context.nodeType = FLAGS_mode;
    context.operationType = "send";
    context.batchSize = FLAGS_batch;
    context.numThreads = 1;

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
    server.init();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    logger.start();
    server.startTimer();

    int numTxRetries = 0;
    int numCqObtained = 0;
    int cqTry = FLAGS_batch * FLAGS_cq_try;

    while (true)
    {
        common::operationCounter++;
        ret = server.postTx();
        while (ret == -FI_EAGAIN)
        {
            if (numCqObtained < common::operationCounter)
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
        if ((common::operationCounter - numCqObtained) > FLAGS_batch)
        {
            ret = server.getNTxCompletion(cqTry);
            if (ret)
            {
                printf("SERVER: getCqCompletion failed\n\n");
                exit(1);
            }
            numCqObtained += cqTry;
        }

        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }

    ret = server.getNTxCompletion(common::operationCounter - numCqObtained);
    if (ret)
    {
        printf("SERVER: getCqCompletion failed\n\n");
        exit(1);
    }

    server.stopTimer();
    logger.stop();

    server.showTransferStatistics(common::operationCounter, 1);
}

void startLatencyTestServer()
{
    int ret;
    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
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

void startCapsTestServer()
{
}

void startRmaServer()
{
    int ret;
    BenchmarkContext context;
    context.experimentName = FLAGS_benchmark_type;
    context.endpoint = FLAGS_endpoint;
    context.provider = FLAGS_provider;
    context.msgSize = FLAGS_payload;
    context.nodeType = FLAGS_mode;
    context.operationType = FLAGS_rma_op;
    context.batchSize = 1;
    context.numThreads = 1;

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    logger.start();
    server.startTimer();
    while (true)
    {
        common::operationCounter += 1;
        ret = server.rma();
        if (ret)
            return;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    server.stopTimer();
    logger.stop();

    server.showTransferStatistics(common::operationCounter, 1);
}

void startRmaBatchServer()
{
    int ret;
    BenchmarkContext context;
    context.experimentName = FLAGS_benchmark_type;
    context.endpoint = FLAGS_endpoint;
    context.provider = FLAGS_provider;
    context.msgSize = FLAGS_payload;
    context.nodeType = FLAGS_mode;
    context.operationType = FLAGS_rma_op;
    context.batchSize = FLAGS_batch;
    context.numThreads = 1;

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    logger.start();
    server.startTimer();
    int numTxRetries = 0;
    int numCqObtained = 0;
    int cqTry = FLAGS_batch * FLAGS_cq_try;

    while (true)
    {
        common::operationCounter++;
        ret = server.postRma();
        while (ret == -FI_EAGAIN)
        {
            if (numCqObtained < common::operationCounter)
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
        if ((common::operationCounter - numCqObtained) > FLAGS_batch)
        {
            ret = server.getNTxCompletion(cqTry);
            if (ret)
            {
                printf("SERVER: getCqCompletion failed\n\n");
                exit(1);
            }
            numCqObtained += cqTry;
        }
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }

    ret = server.getNTxCompletion(common::operationCounter - numCqObtained);
    if (ret)
    {
        printf("SERVER: getCqCompletion failed\n\n");
        exit(1);
    }

    server.stopTimer();
    logger.stop();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(common::operationCounter, 1);
}

void startRmaInjectServer()
{
    int ret;
    BenchmarkContext context;
    context.experimentName = FLAGS_benchmark_type;
    context.endpoint = FLAGS_endpoint;
    context.provider = FLAGS_provider;
    context.msgSize = FLAGS_payload;
    context.nodeType = FLAGS_mode;
    context.operationType = FLAGS_rma_op;
    context.batchSize = 1;
    context.numThreads = 1;

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    logger.start();
    server.startTimer();
    while (true)
    {
        common::operationCounter += 1;
        ret = server.postRmaInject();
        if (ret)
            return;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    server.stopTimer();
    logger.stop();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(common::operationCounter, 1);
}

void startRmaSelectiveCompletionServer()
{
    int ret;
    BenchmarkContext context;
    context.experimentName = FLAGS_benchmark_type;
    context.endpoint = FLAGS_endpoint;
    context.provider = FLAGS_provider;
    context.msgSize = FLAGS_payload;
    context.nodeType = FLAGS_mode;
    context.operationType = FLAGS_rma_op;
    context.batchSize = FLAGS_batch;
    context.numThreads = 1;

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
    server.enableSelectiveCompletion();
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    logger.start();
    server.startTimer();
    int numPendingRequests = 0;
    while (true)
    {
        common::operationCounter += 1;
        if (numPendingRequests == (FLAGS_batch - 1))
        {
            server.postRmaSelectiveComp(true);
            server.getTxCompletion();
            numPendingRequests = 0;
            continue;
        }
        server.postRmaSelectiveComp(false);
        numPendingRequests++;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    server.stopTimer();
    logger.stop();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(common::operationCounter, 1);
}