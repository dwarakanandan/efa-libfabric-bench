#include "CsvLogger.h"
#include "RmaNode.h"

class RmaServer : public RmaNode
{
private:
    void _rmaWorker(size_t workerId);

    void _batchWorker(size_t workerId);

public:
    void rma();

    void inject();

    void batch();

    void batchLargeBuffer();

    void batchSelectiveCompletion();
};