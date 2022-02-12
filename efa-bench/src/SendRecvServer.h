#include "CsvLogger.h"
#include "SendRecvNode.h"

class SendRecvServer : public SendRecvNode
{
private:
    void _pingPongWorker(size_t workerId);

    void _batchWorker(size_t workerId);

    void _latencyWorker(size_t workerId, int warmup_time);

    void _trafficGenerator(size_t workerId);

public:
    void pingPong();

    void pingPongInject();

    void batch();

    void batchLargeBuffer();

    void latency();

    void capabilityTest();

    void saturationLatency();
};
