#include "CsvLogger.h"

CsvLogger::CsvLogger(BenchmarkContext context)
{
    this->context = context;
    headerFields.push_back("timestamp");
    headerFields.push_back("experiment_name");
    headerFields.push_back("provider");
    headerFields.push_back("endpoint");
    headerFields.push_back("node_type");
    headerFields.push_back("batch_size");
    headerFields.push_back("thread_count");
    headerFields.push_back("operation_type");
    headerFields.push_back("message_size");
    headerFields.push_back("ops_per_sec");
    headerFields.push_back("tx_bandwidth");
    headerFields.push_back("rx_bandwidth");
}

void CsvLogger::start()
{
    this->benchmarkRunning = true;
    this->lThread = std::thread(&CsvLogger::loggerTask, this);
}

void CsvLogger::stop()
{
    this->benchmarkRunning = false;
    this->lThread.join();
}

void CsvLogger::loggerTask()
{
    this->logHeader();
    int timestamp = 0;
    while (this->benchmarkRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        this->logRow(timestamp++, common::iterationCounter, 0,0);
    }
}

void CsvLogger::logHeader()
{
    int i;
    for (i = 0; i < headerFields.size() - 1; i++)
    {
        std::cout << headerFields[i] << ",";
    }
    std::cout << headerFields[i] << std::endl;
}

void CsvLogger::logRow(int timestamp, double opsPerSecond, double rxBw, double txBw)
{
    std::cout << timestamp << ",";
    std::cout << this->context.experimentName << ",";
    std::cout << this->context.provider << ",";
    std::cout << this->context.endpoint << ",";
    std::cout << this->context.nodeType << ",";
    std::cout << this->context.batchSize << ",";
    std::cout << this->context.numThreads << ",";
    std::cout << this->context.operationType << ",";
    std::cout << this->context.msgSize << ",";
    std::cout << opsPerSecond << ",";
    std::cout << txBw << ",";
    std::cout << rxBw;
    std::cout << std::endl;
}