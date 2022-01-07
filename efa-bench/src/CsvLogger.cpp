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

uint64_t CsvLogger::getCounter(std::string counter)
{
    char data[20];
    std::ifstream infile;
    infile.open(FLAGS_hw_counters.c_str() + counter);
    infile >> data;
    infile.close();
    return std::stoull(data);
}

double CsvLogger::calculateBandwidthMbps(uint64_t initial, uint64_t current, int timeElapsed)
{
    return (current - initial) / (timeElapsed * 1024.0 * 1024.0);
}

void CsvLogger::loggerTask()
{
    std::stringstream ss;
    ss << std::time(0);
    this->statsFile.open("stats_" + ss.str() + ".csv");

    ss = this->logHeader();
    this->statsFile << ss.str();
    std::cout << ss.str();

    int timestamp = 0;
    this->initialTxBytes = this->getCounter("tx_bytes");
    this->initialRxBytes = this->getCounter("rx_bytes");

    while (this->benchmarkRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        timestamp++;
        double txBw = this->calculateBandwidthMbps(this->initialTxBytes, this->getCounter("tx_bytes"), timestamp);
        double rxBw = this->calculateBandwidthMbps(this->initialRxBytes, this->getCounter("rx_bytes"), timestamp);
        double opsPsec = (common::operationCounter * 1.0) / timestamp;
        ss = this->logRow(timestamp, opsPsec, txBw, rxBw);
        this->statsFile << ss.str();
        std::cout << ss.str();
    }

    this->statsFile.close();
}

std::stringstream CsvLogger::logHeader()
{
    std::stringstream ss;
    int i;
    for (i = 0; i < headerFields.size() - 1; i++)
    {
        ss << headerFields[i] << ",";
    }
    ss << headerFields[i] << std::endl;
    return ss;
}

std::stringstream CsvLogger::logRow(int timestamp, double opsPerSecond, double rxBw, double txBw)
{
    std::stringstream ss;
    ss << timestamp << ",";
    ss << this->context.experimentName << ",";
    ss << this->context.provider << ",";
    ss << this->context.endpoint << ",";
    ss << this->context.nodeType << ",";
    ss << this->context.batchSize << ",";
    ss << this->context.numThreads << ",";
    ss << this->context.operationType << ",";
    ss << this->context.msgSize << ",";
    ss << opsPerSecond << ",";
    ss << txBw << ",";
    ss << rxBw;
    ss << std::endl;
    return ss;
}