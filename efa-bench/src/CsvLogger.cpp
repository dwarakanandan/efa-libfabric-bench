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
    headerFields.push_back("ops_psec");
    headerFields.push_back("tx_pkts_psec");
    headerFields.push_back("rx_pkts_psec");
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

double CsvLogger::calculatePktsPsec(uint64_t initial, uint64_t current, int timeElapsed)
{
    return (current - initial) * 1.0 / (timeElapsed);
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
    this->initialTxPkts = this->getCounter("tx_pkts");
    this->initialRxPkts = this->getCounter("rx_pkts");

    while (this->benchmarkRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        timestamp++;

        double txBw = this->calculateBandwidthMbps(this->initialTxBytes, this->getCounter("tx_bytes"), timestamp);
        double rxBw = this->calculateBandwidthMbps(this->initialRxBytes, this->getCounter("rx_bytes"), timestamp);
        
        double txPktsPsec = this->calculatePktsPsec(this->initialTxPkts, this->getCounter("tx_pkts"), timestamp);
        double rxPktsPsec = this->calculatePktsPsec(this->initialRxPkts, this->getCounter("rx_pkts"), timestamp);

        double opsPsec = (common::operationCounter * 1.0) / timestamp;

        ss = this->logRow(timestamp, opsPsec, txPktsPsec, rxPktsPsec, txBw, rxBw);
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

std::stringstream CsvLogger::logRow(int timestamp, double opsPerSecond, double txPktsPsec, double rxPktsPsec, double txBw, double rxBw)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
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
    ss << txPktsPsec << ",";
    ss << rxPktsPsec << ",";
    ss << txBw << ",";
    ss << rxBw;
    ss << std::endl;
    return ss;
}