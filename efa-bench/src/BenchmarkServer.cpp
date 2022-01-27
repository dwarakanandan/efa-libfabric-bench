#include "BenchmarkServer.h"

using namespace std;
using namespace libefa;

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
    context.numThreads = FLAGS_threads;

    CsvLogger logger = CsvLogger(context);

    int ret;
    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
    server.init();
    server.sync();
    server.initTxBuffer(FLAGS_payload);

    logger.start();
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
    logger.stop();

    server.showTransferStatistics(common::operationCounter / 2, 2);
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
    context.numThreads = FLAGS_threads;

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

void batchServerWorker(std::string port, std::string oobPort)
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

    int numCqObtained = 0;
    int cqTry = FLAGS_batch * FLAGS_cq_try;
    if (cqTry < 1)
    {
        cqTry = 1;
    }
    uint64_t localcounter = 0;
    while (true)
    {
        common::operationCounter++;
        localcounter++;
        ret = server.postTx();
        if (ret)
            return;
        if ((localcounter - numCqObtained) >= FLAGS_batch)
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

    ret = server.getNTxCompletion(localcounter - numCqObtained);
    if (ret)
    {
        printf("SERVER: getCqCompletion failed\n\n");
        exit(1);
    }

    server.stopTimer();

    server.showTransferStatistics(localcounter, 1);
}

void startBatchServer()
{

    BenchmarkContext context;
    context.experimentName = FLAGS_benchmark_type;
    context.endpoint = FLAGS_endpoint;
    context.provider = FLAGS_provider;
    context.msgSize = FLAGS_payload;
    context.nodeType = FLAGS_mode;
    context.operationType = "send";
    context.batchSize = FLAGS_batch;
    context.numThreads = FLAGS_threads;

    std::vector<std::thread> workers;

    for (size_t i = 0; i < FLAGS_threads; i++)
    {
        workers.push_back(std::thread(batchServerWorker, std::to_string(10000 + i), std::to_string(9000 + i)));
    }

    CsvLogger logger = CsvLogger(context);
    logger.start();

    for (std::thread &worker : workers)
    {
        worker.join();
    }

    logger.stop();
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
    context.numThreads = FLAGS_threads;

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
    context.numThreads = FLAGS_threads;

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
    if (cqTry < 1)
    {
        cqTry = 1;
    }

    while (true)
    {
        common::operationCounter++;
        ret = server.postRma(&server.ctx.remote);
        if ((common::operationCounter - numCqObtained) >= FLAGS_batch)
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
    context.numThreads = FLAGS_threads;

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
    context.numThreads = FLAGS_threads;

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
    int cqTry = FLAGS_batch * FLAGS_cq_try;
    if (cqTry < 1)
    {
        cqTry = 1;
    }

    while (true)
    {
        if (numPendingRequests == cqTry - 1)
        {
            server.postRmaSelectiveComp(true);
        }
        else
        {
            server.postRmaSelectiveComp(false);
        }
        numPendingRequests++;
        common::operationCounter += 1;
        if (numPendingRequests > FLAGS_batch)
        {
            server.getTxCompletion();
            numPendingRequests = FLAGS_batch - cqTry;
        }
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    server.stopTimer();
    logger.stop();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(common::operationCounter, 1);
}

void startRmaLargeBufferServer()
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
    context.numThreads = FLAGS_threads;

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
    server.enableLargeBufferInit(common::LARGE_BUFFER_SIZE_GBS);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    fi_rma_iov rma_iov = {0};
    rma_iov.addr = server.ctx.remote.addr;
    rma_iov.key = server.ctx.remote.key;
    rma_iov.len = server.ctx.remote.len;

    uint64_t offsets[common::NUM_OFFSET_ADDRS];
    common::generateRandomOffsets(offsets);
    uint64_t addresses[common::NUM_OFFSET_ADDRS];

    for (size_t i = 0; i < common::NUM_OFFSET_ADDRS; i++)
    {
        addresses[i] = server.ctx.remote.addr + offsets[i];
    }

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    logger.start();
    server.startTimer();

    int addressIndex = 0;
    int numTxRetries = 0;
    int numCqObtained = 0;
    int cqTry = FLAGS_batch * FLAGS_cq_try;
    if (cqTry < 1)
    {
        cqTry = 1;
    }

    while (true)
    {
        common::operationCounter++;

        // Pick a random address within the 20GB memory region
        if (addressIndex == common::NUM_OFFSET_ADDRS - 1)
            addressIndex = 0;
        rma_iov.addr = addresses[addressIndex];

        ret = server.postRma(&rma_iov);
        if ((common::operationCounter - numCqObtained) >= FLAGS_batch)
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

void startBatchLargeBufferServer()
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
    context.numThreads = FLAGS_threads;

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
    server.enableLargeBufferInit(common::LARGE_BUFFER_SIZE_GBS);
    server.init();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    uint64_t offsets[common::NUM_OFFSET_ADDRS];
    common::generateRandomOffsets(offsets);
    char *addresses[common::NUM_OFFSET_ADDRS];

    for (size_t i = 0; i < common::NUM_OFFSET_ADDRS; i++)
    {
        addresses[i] = (char *)server.ctx.tx_buf + offsets[i];
    }

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    logger.start();
    server.startTimer();

    int numCqObtained = 0;
    int addressIndex = 0;
    int cqTry = FLAGS_batch * FLAGS_cq_try;
    if (cqTry < 1)
    {
        cqTry = 1;
    }

    while (true)
    {
        common::operationCounter++;
        if (addressIndex == common::NUM_OFFSET_ADDRS - 1)
            addressIndex = 0;

        ret = server.postTxBuffer(addresses[addressIndex]);
        if (ret)
            return;
        if ((common::operationCounter - numCqObtained) >= FLAGS_batch)
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