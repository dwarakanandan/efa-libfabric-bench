#include "BenchmarkNode.h"

using namespace std;
using namespace libefa;
using namespace common;

SendRecvNode *BenchmarkNode::getSendRecvNode()
{
    SendRecvNode *node;
    if (common::isNode(FLAGS_mode, T::SERVER))
        node = new SendRecvServer();
    else
        node = new SendRecvClient();
    return node;
}

RmaNode *BenchmarkNode::getRmaNode()
{
    RmaNode *node;
    if (common::isNode(FLAGS_mode, T::SERVER))
        node = new RmaServer();
    else
        node = new RmaClient();
    return node;
}

void BenchmarkNode::startNode(SendRecvNode *node)
{
    if (isBenchmark(FLAGS_benchmark_type, T::LATENCY))
        node->latency();
    else if (isBenchmark(FLAGS_benchmark_type, T::INJECT))
        node->pingPongInject();
    else if (isBenchmark(FLAGS_benchmark_type, T::BATCH))
        node->batch();
    else if (isBenchmark(FLAGS_benchmark_type, T::CAPABILITY_TEST))
        node->capabilityTest();
    else if (isBenchmark(FLAGS_benchmark_type, T::PING_PONG))
        node->pingPong();
    else if (isBenchmark(FLAGS_benchmark_type, T::BATCH_LARGE_BUFFER))
        node->batchLargeBuffer();
}

void BenchmarkNode::startNode(RmaNode *node)
{
    if (isBenchmark(FLAGS_benchmark_type, T::RMA))
        node->rma();
    else if (isBenchmark(FLAGS_benchmark_type, T::RMA_BATCH))
        node->batch();
    else if (isBenchmark(FLAGS_benchmark_type, T::RMA_INJECT))
        node->inject();
    else if (isBenchmark(FLAGS_benchmark_type, T::RMA_SEL_COMP))
        node->batchSelectiveCompletion();
    else if (isBenchmark(FLAGS_benchmark_type, T::RMA_LARGE_BUFFER))
        node->batchLargeBuffer();
}

void BenchmarkNode::run()
{
    if (isBenchmarkClassSendRecv(FLAGS_benchmark_type))
    {
        startNode(getSendRecvNode());
    }
    else if (isBenchmarkClassRma(FLAGS_benchmark_type))
    {
        startNode(getRmaNode());
    }
}

void BenchmarkNode::getFabInfo()
{
    fi_info *hints = fi_allocinfo();
    common::setBaseFabricHints(hints);
    Server server = Server(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port),
                           std::to_string(FLAGS_oob_port), hints);
    server.printFabricInfo();
}
