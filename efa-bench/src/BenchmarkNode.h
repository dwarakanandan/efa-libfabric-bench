#include "Common.h"

#include "SendRecvServer.h"
#include "SendRecvClient.h"
#include "RmaServer.h"
#include "RmaClient.h"

class BenchmarkNode
{
    SendRecvNode *getSendRecvNode();

    RmaNode *getRmaNode();

    void startNode(SendRecvNode *node);

    void startNode(RmaNode *node);

public:
    void run();

    void getFabInfo();
};