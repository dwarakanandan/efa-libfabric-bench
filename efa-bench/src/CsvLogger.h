#include "Common.h"

class CsvLogger
{
    BenchmarkContext context;

    std::thread lThread;

    bool benchmarkRunning;

    std::vector<std::string> headerFields;

public:
    CsvLogger(BenchmarkContext context);

    void loggerTask();

    void start();

    void stop();

    void logHeader();

    void logRow(int timestamp, double opsPerSecond, double rxBw, double txBw);
};
