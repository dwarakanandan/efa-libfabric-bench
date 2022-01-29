#pragma once

#include "Common.h"

struct CsvStat
{
    int timestamp;
    double opsPerSecond;
    double txPktsPsec;
    double rxPktsPsec;
    double txBw;
    double rxBw;
    double appBw;
    double latency;
};

class CsvLogger
{
    BenchmarkContext context;

    std::thread lThread;

    std::vector<std::string> headerFields;

    uint64_t initialTxBytes;

    uint64_t initialRxBytes;

    uint64_t initialTxPkts;

    uint64_t initialRxPkts;

    std::ofstream statsFile;

public:
    CsvLogger(BenchmarkContext context);

    void loggerTask();

    void start();

    void stop();

    std::stringstream logHeader();

    uint64_t getCounter(std::string counter);

    bool getAggregateConnectionStatus();

    uint64_t getAggregateOpsCounter();

    double calculateBandwidthMbps(uint64_t initial, uint64_t current, int timeElapsed);

    double calculatePktsPsec(uint64_t initial, uint64_t current, int timeElapsed);

    std::stringstream logRow(CsvStat stat);
};
