#pragma once

#include "includes.h"
#include "ConnectionContext.h"

namespace libefa
{
    class ConnectionContext;

    class FabricUtil
    {
        static int allocMessages(ConnectionContext *ctx);

        static int cqReadError(struct fid_cq *cq);

        static uint64_t initCqData(struct fi_info *info);

    public:
        static uint64_t getTimeMicroSeconds();

        static int openFabricRes(ConnectionContext *ctx);

        static int allocActiveRes(ConnectionContext *ctx);

        static int initEndpoint(ConnectionContext *ctx);

        static int sendName(ConnectionContext *ctx, struct fid *endpoint);

        static int receiveName(ConnectionContext *ctx);

        static int ctrlSend(ConnectionContext *ctx, char *buf, size_t size);

        static int ctrlReceive(ConnectionContext *ctx, char *buf, size_t size);

        static int ctrlReceiveString(ConnectionContext *ctx, char *buf, size_t size);

        static int insertAddressVector(struct fid_av *av, void *addr, size_t count, fi_addr_t *fi_addr, uint64_t flags, void *context);

        static void prinAddrinfo(struct addrinfo *ai, std::string msg);

        static ssize_t rx(ConnectionContext *ctx, struct fid_ep *ep, size_t size);

        static ssize_t tx(ConnectionContext *ctx, struct fid_ep *ep, size_t size);

        static void fillBuffer(void *buf, int size);

        static int getCqCompletion(struct fid_cq *cq, uint64_t *cur, uint64_t total, int timeout_sec);

        static ssize_t postRx(ConnectionContext *ctx, struct fid_ep *ep, size_t size, void *ctxptr);

        static int getRxCompletion(ConnectionContext *ctx, uint64_t total);

        static ssize_t postTx(ConnectionContext *ctx, struct fid_ep *ep, size_t size, void *ctxptr);

        static int getTxCompletion(ConnectionContext *ctx, uint64_t total);
    };
}
