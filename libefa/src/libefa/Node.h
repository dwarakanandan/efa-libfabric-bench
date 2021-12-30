#pragma once

#include "includes.h"
#include "ftlib/shared.h"

namespace libefa
{
    class Node
    {
    protected:
        Node(std::string provider, std::string endpoint, fi_info *userHints);

        ~Node();

    public:
        int init();

        int sync();

        int initTxBuffer(size_t size);

        int postTx();

        int getTxCompletion();

        int tx();

        int inject();

        int rx();

        void startTimer();

        void stopTimer();

        void showTransferStatistics(int iterations, int transfersPerIterations);

        int exchangeKeys();

        int initRmaOp(std::string operation);

        int postRma();

        int rma();

        int printFabricInfo();
    };
}