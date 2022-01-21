#pragma once

#include "includes.h"
#include "ftlib/shared.h"

namespace libefa
{
    class Node
    {
    protected:
        Node(std::string provider, std::string endpoint, std::string oobPort, fi_info *userHints);

        ~Node();

    public:
        struct ConnectionContext ctx;

        int init();

        int initMultiRecv();

        int postMultiRecv(int n);

        int sync();

        int initTxBuffer(size_t size);

        int postTx();

        int getTxCompletion();

        int getTxCompletionWithTimeout(int timeout);

        int getNTxCompletion(int n);

        int tx();

        int inject();

        int postRx(void *buffer);

        int getRxCompletion();

        int getNRxCompletion(int n);

        int rx();

        void startTimer();

        void stopTimer();

        void showTransferStatistics(int iterations, int transfersPerIterations);

        int exchangeKeys();

        int initRmaOp(std::string operation);

        int postRma();

        int postRmaInject();

        int postRmaSelectiveComp(bool enableCompletion);

        int enableSelectiveCompletion();

        int rma();

        int printFabricInfo();
    };
}