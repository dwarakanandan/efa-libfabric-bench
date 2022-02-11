#include "SendRecvNode.h"

class SendRecvClient : public SendRecvNode
{
private:
    void _pingPongWorker(size_t workerId);

    void _batchWorker(size_t workerId);

    void _latencyWorker(size_t workerId);

    void _trafficGenerator(size_t workerId);

public:
    void pingPong();

    void pingPongInject();

    void batch();

    void batchLargeBuffer();

    void latency();

    void capabilityTest();

    void multiRecvBatch();

    void saturationLatency();
};
