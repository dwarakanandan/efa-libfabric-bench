#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numeric>

#include <libefa/Server.h>
#include <libefa/Client.h>

#include "GflagsConfig.h"

enum T
{
    SERVER,
    CLIENT,
    PING_PONG,
    INJECT,
    LATENCY,
    BATCH,
    BATCH_SELECTIVE_COMP,
    CAPABILITY_TEST,
    RMA,
    RMA_BATCH,
    RMA_INJECT,
    RMA_SELECTIVE_COMP,
    RMA_LARGE_BUFFER,
    BATCH_LARGE_BUFFER,
    SATURATION_LATENCY
};

static std::map<T, const char *> BENCHMARK_TYPE = {
    {PING_PONG, "ping_pong"},
    {INJECT, "inject"},
    {LATENCY, "latency"},
    {BATCH, "batch"},
    {BATCH_SELECTIVE_COMP, "batch_sel_comp"},
    {CAPABILITY_TEST, "caps"},
    {RMA, "rma"},
    {RMA_BATCH, "rma_batch"},
    {RMA_INJECT, "rma_inject"},
    {RMA_SELECTIVE_COMP, "rma_sel_comp"},
    {RMA_LARGE_BUFFER, "rma_large_buffer"},
    {BATCH_LARGE_BUFFER, "batch_large_buffer"},
    {SATURATION_LATENCY, "sat_latency"},
};

static std::map<T, const char *> NODE_TYPE = {
    {SERVER, "server"},
    {CLIENT, "client"},
};

struct BenchmarkContext
{
    std::string experimentName;
    std::string provider;
    std::string endpoint;
    std::string nodeType;
    uint64_t batchSize;
    uint64_t numThreads;
    std::string operationType;
    uint64_t msgSize;
    uint64_t xfersPerIter;
    uint64_t iterations;
};

namespace common
{
    bool isBenchmark(std::string t1, T t2);

    bool isBenchmarkClassSendRecv(std::string t1);

    bool isBenchmarkClassRma(std::string t1);

    bool isNode(std::string t1, T t2);

    void setBaseFabricHints(fi_info *hints);

    void setRmaFabricHints(fi_info *hints);

    void initBenchmarkContext(BenchmarkContext *context);

    std::map<int, int> getPayloadIterMap();

    inline size_t NUM_OFFSET_ADDRS = 1000000;

    inline size_t LARGE_BUFFER_SIZE_GBS = 20;

    inline std::vector<std::thread> workers;

    inline std::vector<uint64_t> workerOperationCounter;

    inline std::vector<bool> workerConnectionStatus;

    void generateRandomOffsets(uint64_t *offsets);

    void waitFor(int usec);
}
