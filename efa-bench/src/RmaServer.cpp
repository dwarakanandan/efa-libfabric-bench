#include "RmaServer.h"

using namespace std;
using namespace libefa;

void RmaServer::_rmaWorker(size_t workerId)
{
    int ret;
    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);
    common::workerConnectionStatus[workerId] = true;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    server.startTimer();
    while (true)
    {
        common::workerOperationCounter[workerId] += 1;
        ret = server.rma();
        if (ret)
            return;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    server.stopTimer();

    common::workerConnectionStatus[workerId] = false;
    server.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}

void RmaServer::rma()
{
    int ret;
    BenchmarkContext context;
    common::initBenchmarkContext(&context);
    context.operationType = FLAGS_rma_op;

    for (size_t i = 0; i < FLAGS_threads; i++)
    {
        common::workerConnectionStatus.push_back(false);
        common::workerOperationCounter.push_back(0);
        common::workers.push_back(std::thread(&RmaServer::_rmaWorker, this, i));
    }

    CsvLogger logger = CsvLogger(context);
    logger.start();

    for (std::thread &worker : common::workers)
    {
        worker.join();
    }

    logger.stop();
}

void RmaServer::_batchWorker(size_t workerId)
{
    int ret;
    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    common::workerConnectionStatus[workerId] = true;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

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
        common::workerOperationCounter[workerId]++;
        ret = server.postRma(&server.ctx.remote);
        if ((common::workerOperationCounter[workerId] - numCqObtained) >= FLAGS_batch)
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

    ret = server.getNTxCompletion(common::workerOperationCounter[workerId] - numCqObtained);
    if (ret)
    {
        printf("SERVER: getCqCompletion failed\n\n");
        exit(1);
    }

    server.stopTimer();

    common::workerConnectionStatus[workerId] = false;

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}

void RmaServer::batch()
{
    int ret;
    BenchmarkContext context;
    common::initBenchmarkContext(&context);
    context.operationType = FLAGS_rma_op;

    for (size_t i = 0; i < FLAGS_threads; i++)
    {
        common::workerConnectionStatus.push_back(false);
        common::workerOperationCounter.push_back(0);
        common::workers.push_back(std::thread(&RmaServer::_batchWorker, this, i));
    }

    CsvLogger logger = CsvLogger(context);
    logger.start();

    for (std::thread &worker : common::workers)
    {
        worker.join();
    }

    logger.stop();
}

void RmaServer::inject()
{
    int ret;
    BenchmarkContext context;
    common::initBenchmarkContext(&context);
    context.operationType = FLAGS_rma_op;

    size_t workerId = 0;
    common::workerConnectionStatus.push_back(false);
    common::workerOperationCounter.push_back(0);

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    common::workerConnectionStatus[workerId] = true;

    server.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    logger.start();
    server.startTimer();
    while (true)
    {
        common::workerOperationCounter[workerId] += 1;
        ret = server.postRmaInject();
        if (ret)
            return;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    server.stopTimer();

    common::workerConnectionStatus[workerId] = false;
    logger.stop();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}

void RmaServer::batchSelectiveCompletion()
{
    int ret;
    BenchmarkContext context;
    common::initBenchmarkContext(&context);
    context.operationType = FLAGS_rma_op;

    size_t workerId = 0;
    common::workerConnectionStatus.push_back(false);
    common::workerOperationCounter.push_back(0);

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
    server.enableSelectiveCompletion();
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    common::workerConnectionStatus[workerId] = true;

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
        common::workerOperationCounter[workerId] += 1;
        if (numPendingRequests > FLAGS_batch)
        {
            server.getTxCompletion();
            numPendingRequests = FLAGS_batch - cqTry;
        }
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    server.stopTimer();

    common::workerConnectionStatus[workerId] = false;
    logger.stop();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}

void RmaServer::batchLargeBuffer()
{
    int ret;
    BenchmarkContext context;
    common::initBenchmarkContext(&context);
    context.operationType = FLAGS_rma_op;

    size_t workerId = 0;
    common::workerConnectionStatus.push_back(false);
    common::workerOperationCounter.push_back(0);

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setRmaFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
    server.enableLargeBufferInit(common::LARGE_BUFFER_SIZE_GBS);
    server.initRmaOp(FLAGS_rma_op);

    server.init();
    server.exchangeKeys();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    common::workerConnectionStatus[workerId] = true;

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
        common::workerOperationCounter[workerId]++;

        // Pick a random address within the 20GB memory region
        if (addressIndex == common::NUM_OFFSET_ADDRS - 1)
            addressIndex = 0;
        rma_iov.addr = addresses[addressIndex];

        ret = server.postRma(&rma_iov);
        if ((common::workerOperationCounter[workerId] - numCqObtained) >= FLAGS_batch)
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

    ret = server.getNTxCompletion(common::workerOperationCounter[workerId] - numCqObtained);
    if (ret)
    {
        printf("SERVER: getCqCompletion failed\n\n");
        exit(1);
    }

    server.stopTimer();

    common::workerConnectionStatus[workerId] = false;
    logger.stop();

    // Sync after RMA ops are complete
    server.sync();

    server.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}