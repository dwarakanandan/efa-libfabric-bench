#include "Common.h"

class CsvLogger
{
    BenchmarkContext context;

    std::thread lThread;

    bool benchmarkRunning;

    std::vector<std::string> headerFields;

    uint64_t initialTxBytes;

    uint64_t initialRxBytes;

    std::ofstream statsFile;

public:
    CsvLogger(BenchmarkContext context);

    void loggerTask();

    void start();

    void stop();

    std::stringstream logHeader();

    uint64_t getCounter(std::string counter);

    double calculateBandwidthMbps(uint64_t initial, uint64_t current, int timeElapsed);

    std::stringstream logRow(int timestamp, double opsPerSecond, double rxBw, double txBw);
};
