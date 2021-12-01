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

        bool isTagged;

        uint16_t port;

        virtual int initFabric(fi_info* hints) = 0;

        virtual int ctrlInit() = 0;

        virtual int ctrlSync() = 0;

    public:
        virtual ConnectionContext getConnectionContext() = 0;

        virtual int init(fi_info* hints) = 0;

        virtual ~Node() {};
    };
}