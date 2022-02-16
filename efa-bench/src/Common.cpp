#include "Common.h"

bool common::isBenchmark(std::string t1, T t2)
{
    return t1.compare(BENCHMARK_TYPE[t2]) == 0;
}

bool common::isBenchmarkClassSendRecv(std::string t1)
{
    return !common::isBenchmarkClassRma(t1);
}

bool common::isBenchmarkClassRma(std::string t1)
{
    return t1.compare(BENCHMARK_TYPE[T::RMA]) == 0 ||
           t1.compare(BENCHMARK_TYPE[T::RMA_BATCH]) == 0 ||
           t1.compare(BENCHMARK_TYPE[T::RMA_INJECT]) == 0 ||
           t1.compare(BENCHMARK_TYPE[T::RMA_SELECTIVE_COMP]) == 0 ||
           t1.compare(BENCHMARK_TYPE[T::RMA_LARGE_BUFFER]) == 0;
}

bool common::isNode(std::string t1, T t2)
{
    return t1.compare(NODE_TYPE[t2]) == 0;
}

std::map<int, int> common::getPayloadIterMap()
{
    std::map<int, int> payloadIterationMap;
    payloadIterationMap.insert(std::make_pair(1, 1000000));
    payloadIterationMap.insert(std::make_pair(8, 1000000));
    payloadIterationMap.insert(std::make_pair(64, 1000000));
    payloadIterationMap.insert(std::make_pair(512, 1000000));
    payloadIterationMap.insert(std::make_pair(1024, 500000));
    payloadIterationMap.insert(std::make_pair(4096, 500000));
    payloadIterationMap.insert(std::make_pair(8192, 500000));
    return payloadIterationMap;
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

void common::generateRandomOffsets(uint64_t *offsets)
{
    size_t _buffer_size_bytes = (1024 * 1024 * 1024 * common::LARGE_BUFFER_SIZE_GBS) / 8;
    for (size_t i = 0; i < common::NUM_OFFSET_ADDRS; i++)
    {
        offsets[i] = std::rand() % _buffer_size_bytes;
    }
}

void common::initBenchmarkContext(BenchmarkContext *context)
{
    if (FLAGS_tagged)
    {
        context->experimentName = FLAGS_benchmark_type + "_tagged";
    }
    else
    {
        context->experimentName = FLAGS_benchmark_type;
    }
    context->endpoint = FLAGS_endpoint;
    context->provider = FLAGS_provider;
    context->msgSize = FLAGS_payload;
    context->nodeType = FLAGS_mode;
    context->batchSize = FLAGS_batch;
    context->numThreads = FLAGS_threads;
    context->xfersPerIter = 1;
}

void common::waitFor(int nsec)
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    while (true)
    {
        if (std::chrono::steady_clock::now() - start > std::chrono::nanoseconds(nsec))
            break;
    }
}