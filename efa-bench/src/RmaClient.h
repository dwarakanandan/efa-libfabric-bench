#include "RmaNode.h"

class RmaClient : public RmaNode
{
private:
    void _spawnRmaWorkers(size_t numWorkers);

    void _rmaWorker(size_t workerId);

    void _batchWorker(size_t workerId);

public:
    void rma();

    void inject();

    void batch();

    void batchLargeBuffer();

    void batchSelectiveCompletion();
};