#include "SendRecvServer.h"

using namespace std;
using namespace libefa;

void startPingPongServer()
{
    BenchmarkContext context;
    common::initBenchmarkContext(&context);
    context.operationType = "send";

    size_t workerId = 0;
    common::workerConnectionStatus.push_back(false);
    common::workerOperationCounter.push_back(0);

    CsvLogger logger = CsvLogger(context);
    logger.start();

    int ret;
    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
    server.init();
    server.sync();
    server.initTxBuffer(FLAGS_payload);

    common::workerConnectionStatus[workerId] = true;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    server.startTimer();
    while (true)
    {
        common::workerOperationCounter[workerId] += 2;
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

    common::workerConnectionStatus[workerId] = false;
    logger.stop();

    server.showTransferStatistics(common::workerOperationCounter[workerId] / 2, 2);
}

void startPingPongInjectServer()
{
    int ret;
    BenchmarkContext context;
    common::initBenchmarkContext(&context);
    context.operationType = "inject";

    size_t workerId = 0;
    common::workerConnectionStatus.push_back(false);
    common::workerOperationCounter.push_back(0);

    CsvLogger logger = CsvLogger(context);
    logger.start();

    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
    server.init();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    common::workerConnectionStatus[workerId] = true;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    server.startTimer();
    while (true)
    {
        common::workerOperationCounter[workerId] += 2;
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

    common::workerConnectionStatus[workerId] = false;
    logger.stop();

    server.showTransferStatistics(common::workerOperationCounter[workerId] / 2, 2);
}

void _batchServerWorker(size_t workerId)
{
    int ret;
    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint,
                           std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
    server.init();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    server.startTimer();

    common::workerConnectionStatus[workerId] = true;

    int numCqObtained = 0;
    int cqTry = FLAGS_batch * FLAGS_cq_try;
    if (cqTry < 1)
    {
        cqTry = 1;
    }

    while (true)
    {
        common::workerOperationCounter[workerId]++;

        ret = server.postTx();
        if (ret)
            return;
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

    server.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}

void startBatchServer()
{

    BenchmarkContext context;
    common::initBenchmarkContext(&context);
    context.operationType = "send";

    for (size_t i = 0; i < FLAGS_threads; i++)
    {
        common::workerConnectionStatus.push_back(false);
        common::workerOperationCounter.push_back(0);
        common::workers.push_back(std::thread(_batchServerWorker, i));
    }

    CsvLogger logger = CsvLogger(context);
    logger.start();

    for (std::thread &worker : common::workers)
    {
        worker.join();
    }

    logger.stop();
}

void startLatencyTestServer()
{
    int ret;

    size_t workerId = 0;

    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
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

void startBatchLargeBufferServer()
{
    int ret;
    BenchmarkContext context;
    common::initBenchmarkContext(&context);
    context.operationType = "send";

    size_t workerId = 0;
    common::workerConnectionStatus.push_back(false);
    common::workerOperationCounter.push_back(0);

    CsvLogger logger = CsvLogger(context);

    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);

    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
                           std::to_string(FLAGS_oob_port + workerId), hints);
    server.enableLargeBufferInit(common::LARGE_BUFFER_SIZE_GBS);
    server.init();
    server.sync();

    server.initTxBuffer(FLAGS_payload);

    common::workerConnectionStatus[workerId] = true;

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
        common::workerOperationCounter[workerId]++;
        if (addressIndex == common::NUM_OFFSET_ADDRS - 1)
            addressIndex = 0;

        ret = server.postTxBuffer(addresses[addressIndex]);
        if (ret)
            return;
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

    server.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}