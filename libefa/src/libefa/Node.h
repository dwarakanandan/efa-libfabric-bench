#pragma once

#include "includes.h"
#include "ftlib/shared.h"

namespace libefa
{
    class Node
    {
    private:
        size_t txPayloadSize;
    public:
        virtual int init();

        virtual int sync();

        virtual int initTxBuffer(size_t size);

        virtual int postTx();

        virtual int getTxCompletion();

        virtual int tx();

        virtual int inject();

        virtual int rx();

        virtual void startTimer();

        virtual void stopTimer();

        virtual void showTransferStatistics(int iterations, int transfersPerIterations);

        virtual int exchangeKeys();

        virtual int initRmaOp(std::string operation);

        virtual int postRma();

        virtual int rma();

        virtual ~Node();
    };
}