#pragma once

#include "Common.h"

class SendRecvNode
{
public:
    virtual void pingPong() = 0;

    virtual void pingPongInject() = 0;

    virtual void batch() = 0;

    virtual void batchLargeBuffer() = 0;

    virtual void latency() = 0;

    virtual void capabilityTest() = 0;
};