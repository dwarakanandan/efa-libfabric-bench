#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>

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
    CAPS,
    RMA,
    RMA_BATCH,
    RMA_INJECT,
    RMA_SEL_COMP,
    RMA_LARGE_BUFFER,
    BATCH_LARGE_BUFFER,
};

static std::map<T, const char *> BENCHMARK_TYPE = {
    {PING_PONG, "ping_pong"},
    {INJECT, "inject"},
    {LATENCY, "latency"},
    {BATCH, "batch"},
    {CAPS, "caps"},
    {RMA, "rma"},
    {RMA_BATCH, "rma_batch"},
    {RMA_INJECT, "rma_inject"},
    {RMA_SEL_COMP, "rma_sel_comp"},
    {RMA_LARGE_BUFFER, "rma_large_buffer"},
    {BATCH_LARGE_BUFFER, "batch_large_buffer"},
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
};

namespace common
{
    bool is_benchmark(std::string t1, T t2);

    bool is_node(std::string t1, T t2);

    void setBaseFabricHints(fi_info *hints);

    void setRmaFabricHints(fi_info *hints);

    std::map<int, int> getPayloadIterMap();

    inline uint64_t operationCounter = 0;

    inline size_t NUM_OFFSET_ADDRS = 1000000;

    inline size_t LARGE_BUFFER_SIZE_GBS = 20; 

    void generateRandomOffsets(uint64_t *offsets);
}
