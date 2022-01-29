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
    headerFields.push_back("tx_bw_mbps");
    headerFields.push_back("rx_bw_mbps");
    headerFields.push_back("app_bw_mbps");
    headerFields.push_back("latency_usec");
}

void CsvLogger::start()
{
    this->lThread = std::thread(&CsvLogger::loggerTask, this);
}

void CsvLogger::stop()
{
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
    return (current - initial) / (timeElapsed * 1000.0 * 1000.0);
}

double CsvLogger::calculatePktsPsec(uint64_t initial, uint64_t current, int timeElapsed)
{
    return (current - initial) * 1.0 / (timeElapsed);
}

bool CsvLogger::getAggregateConnectionStatus()
{
    bool aggregateState = true;
    for (bool workerState : common::workerConnectionStatus)
    {
        aggregateState = aggregateState && workerState;
    }
    return aggregateState;
}

uint64_t CsvLogger::getAggregateOpsCounter()
{
    uint64_t aggregateOps = 0;
    for (uint64_t workerOps : common::workerOperationCounter)
    {
        aggregateOps += workerOps;
    }
    return aggregateOps;
}

void CsvLogger::loggerTask()
{
    while (this->getAggregateConnectionStatus() == false)
    {
    }

    std::stringstream ss;
    this->statsFile.open(FLAGS_stat_file + ".csv");

    ss = this->logHeader();
    this->statsFile << ss.str();
    std::cout << ss.str();

    int timestamp = 0;
    std::string tx_bytes, rx_bytes, tx_packets, rx_packets;

    if (FLAGS_hw_counters.find("infiniband") != std::string::npos)
    {
        tx_bytes = "tx_bytes";
        rx_bytes = "rx_bytes";
        tx_packets = "tx_pkts";
        rx_packets = "rx_pkts";
    }
    else
    {
        tx_bytes = "tx_bytes";
        rx_bytes = "rx_bytes";
        tx_packets = "tx_packets";
        rx_packets = "rx_packets";
    }

    this->initialTxBytes = this->getCounter(tx_bytes);
    this->initialRxBytes = this->getCounter(rx_bytes);
    this->initialTxPkts = this->getCounter(tx_packets);
    this->initialRxPkts = this->getCounter(rx_packets);

    while (this->getAggregateConnectionStatus() == true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        timestamp++;

        CsvStat stat;
        stat.timestamp = timestamp;

        stat.txBw = this->calculateBandwidthMbps(this->initialTxBytes, this->getCounter(tx_bytes), timestamp);
        stat.rxBw = this->calculateBandwidthMbps(this->initialRxBytes, this->getCounter(rx_bytes), timestamp);

        stat.txPktsPsec = this->calculatePktsPsec(this->initialTxPkts, this->getCounter(tx_packets), timestamp);
        stat.rxPktsPsec = this->calculatePktsPsec(this->initialRxPkts, this->getCounter(rx_packets), timestamp);

        uint64_t aggregateOps = this->getAggregateOpsCounter();
        stat.opsPerSecond = aggregateOps / (timestamp * 1.0);
        stat.appBw = (aggregateOps * this->context.msgSize * this->context.xfersPerIter) / (timestamp * 1000.0 * 1000.0);
        stat.latency = ((timestamp * 1000000.0) / aggregateOps / this->context.xfersPerIter);

        ss = this->logRow(stat);
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

std::stringstream CsvLogger::logRow(CsvStat stat)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << stat.timestamp << ",";
    ss << this->context.experimentName << ",";
    ss << this->context.provider << ",";
    ss << this->context.endpoint << ",";
    ss << this->context.nodeType << ",";
    ss << this->context.batchSize << ",";
    ss << this->context.numThreads << ",";
    ss << this->context.operationType << ",";
    ss << this->context.msgSize << ",";
    ss << stat.opsPerSecond << ",";
    ss << stat.txPktsPsec << ",";
    ss << stat.rxPktsPsec << ",";
    ss << stat.txBw << ",";
    ss << stat.rxBw << ",";
    ss << stat.appBw << ",";
    ss << stat.latency;
    ss << std::endl;
    return ss;
}