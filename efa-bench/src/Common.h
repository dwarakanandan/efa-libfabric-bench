#pragma once

#include <iostream>
#include <vector>
#include <map>

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
};

static std::map<T, const char *> BENCHMARK_TYPE = {
    {PING_PONG, "pingpong"},
    {INJECT, "inject"},
    {LATENCY, "latency"},
    {BATCH, "batch"},
    {CAPS, "caps"},
    {RMA, "rma"},
};

static std::map<T, const char *> NODE_TYPE = {
    {SERVER, "server"},
    {CLIENT, "client"},
};

namespace common
{
    bool is_benchmark(std::string t1, T t2);

    bool is_node(std::string t1, T t2);

    void setBaseFabricHints(fi_info *hints);

    void setRmaFabricHints(fi_info *hints);
}
