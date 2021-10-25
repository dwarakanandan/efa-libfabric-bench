#pragma once

#include "includes.h"
#include "ConnectionContext.h"

namespace libefa
{
    class Node
    {
    protected:
        ConnectionContext ctx;

        std::string provider;

        std::string endpoint;

        virtual int initFabric() = 0;

        virtual int ctrlInit() = 0;

        virtual int ctrlSync() = 0;

    public:
        virtual ConnectionContext getConnectionContext() = 0;

        virtual int init() = 0;

        virtual ~Node() {};
    };
}