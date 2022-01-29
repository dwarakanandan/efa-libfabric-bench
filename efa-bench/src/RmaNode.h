#pragma once

#include "Common.h"

class RmaNode
{
public:
    virtual void rma() = 0;

    virtual void batch() = 0;

    virtual void inject() = 0;

    virtual void batchSelectiveCompletion() = 0;

    virtual void batchLargeBuffer() = 0;
};
