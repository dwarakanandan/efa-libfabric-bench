#pragma once

#include <iostream>
#include <vector>
#include <map>

#include <libefa/FabricInfo.h>
#include <libefa/Server.h>
#include <libefa/Client.h>
#include <libefa/PerformancePrinter.h>

#include "GflagsConfig.h"

enum T
{
    SERVER,
    CLIENT,
    BANDWIDTH,
    INJECT,
    LATENCY,
};

static std::map<T, const char *> BENCHMARK_TYPE = {
    {BANDWIDTH, "bandwidth"},
    {INJECT, "inject"},
    {LATENCY, "latency"},
};

static std::map<T, const char *> NODE_TYPE = {
    {SERVER, "server"},
    {CLIENT, "client"},
};

namespace common
{
    bool is_benchmark(std::string t1, T t2);

    bool is_node(std::string t1, T t2);
}
