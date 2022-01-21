#include "Node.h"

libefa::Node::Node(std::string provider, std::string endpoint, std::string oobPort, fi_info *userHints)
{
    ctx = {0};
    init_connection_context(&ctx);

    ctx.hints = userHints;
    init_opts(&ctx.opts);

    ctx.opts.options |= FT_OPT_BW;

    ctx.hints->addr_format = ctx.opts.address_format;
    ctx.hints->domain_attr->mr_mode = ctx.opts.mr_mode;

    ft_parseinfo('p', strdup(provider.c_str()), ctx.hints, &ctx.opts);
    ft_parseinfo('e', strdup(endpoint.c_str()), ctx.hints, &ctx.opts);

    // Enable out-of-band address exchange for the EFA provider
    if (provider == "efa")
    {
        ctx.opts.options |= FT_OPT_OOB_SYNC;
        ctx.opts.options |= FT_OPT_OOB_ADDR_EXCH;
        ctx.opts.oob_port = strdup(oobPort.c_str());
    }
}

int libefa::Node::enableSelectiveCompletion()
{
    ctx.opts.options |= FT_OPT_SELECTIVE_COMP;
}

int libefa::Node::init()
{
    return ft_init_fabric(&ctx);
}

int libefa::Node::initMultiRecv()
{
    int ret, chunk;
    ret = alloc_ep_res_multi_recv(&ctx);
    if (ret)
        return ret;

    ret = fi_setopt(&ctx.ep->fid, FI_OPT_ENDPOINT, FI_OPT_MIN_MULTI_RECV,
                    &ctx.tx_size, sizeof(ctx.tx_size));
    if (ret)
        return ret;

    for (chunk = 0; chunk < 2; chunk++)
    {
        ret = repost_multi_recv(&ctx, chunk);
        if (ret)
            return ret;
    }

    return ret;
}

int libefa::Node::postMultiRecv(int n)
{
    return wait_for_multi_recv_completion(&ctx, n);
}

int libefa::Node::sync()
{
    return ft_sync(&ctx);
}

libefa::Node::~Node()
{
    ft_free_res(&ctx);
}

int libefa::Node::initTxBuffer(size_t size)
{
    ctx.opts.transfer_size = size;
    return ft_fill_buf(&ctx, (char *)ctx.tx_buf + ft_tx_prefix_size(&ctx), ctx.opts.transfer_size);
}

int libefa::Node::postTx()
{
    return ft_post_tx(&ctx, ctx.ep, ctx.remote_fi_addr, ctx.opts.transfer_size, NO_CQ_DATA, &ctx.tx_ctx);
}

int libefa::Node::getTxCompletionWithTimeout(int timeout)
{
    int ret;
    int ctxTimeoutBackup = ctx.timeout;
    ctx.timeout = timeout;
    ret = ft_get_tx_comp(&ctx, ctx.tx_seq);
    ctx.timeout = ctxTimeoutBackup;
    return ret;
}

int libefa::Node::getTxCompletion()
{
    return ft_get_tx_comp(&ctx, ctx.tx_seq);
}

int libefa::Node::getNTxCompletion(int n)
{
    return ft_get_tx_comp(&ctx, ctx.tx_cq_cntr + n);
}

int libefa::Node::tx()
{
    int ret;
    ret = postTx();
    if (ret)
        return ret;
    return getTxCompletion();
}

int libefa::Node::inject()
{
    return ft_inject(&ctx, ctx.ep, ctx.remote_fi_addr, ctx.opts.transfer_size);
}

int libefa::Node::getRxCompletion()
{
    return ft_get_rx_comp(&ctx, ctx.rx_seq);
}

int libefa::Node::getNRxCompletion(int n)
{
    int ret;
    struct fi_cq_err_entry comp;

    int numObtained = 0;
    while (numObtained < n)
    {
        ret = fi_cq_sread(ctx.rxcq, &comp, n - numObtained, NULL, ctx.timeout);
        if (ret < 0)
            return ret;
        numObtained += ret;
    }

    return EXIT_SUCCESS;
}

int libefa::Node::postRx(void *buffer)
{
    return ft_post_rx_buf(&ctx, ctx.ep, ctx.rx_size, &ctx.rx_ctx, buffer, ctx.mr_desc, ctx.ft_tag);
}

int libefa::Node::rx()
{
    int ret;
    ret = ft_get_rx_comp(&ctx, ctx.rx_seq);
    if (ret)
        return ret;
    return ft_post_rx(&ctx, ctx.ep, ctx.rx_size, &ctx.rx_ctx);
}

void libefa::Node::startTimer()
{
    ft_start(&ctx);
}

void libefa::Node::stopTimer()
{
    ft_stop(&ctx);
}

void libefa::Node::showTransferStatistics(int iterations, int transfersPerIterations)
{
    show_perf(NULL, ctx.opts.transfer_size, iterations, &(ctx.start), &(ctx.end), transfersPerIterations);
}

int libefa::Node::postRma()
{
    return ft_post_rma(&ctx, ctx.opts.rma_op, ctx.ep, ctx.opts.transfer_size, &ctx.remote, NULL);
}

int libefa::Node::postRmaInject()
{
    return ft_post_rma_inject(&ctx, ctx.opts.rma_op, ctx.ep, ctx.opts.transfer_size, &ctx.remote);
}

int libefa::Node::postRmaSelectiveComp(bool enableCompletion)
{
    return ft_post_rma_selective_comp(&ctx, ctx.opts.rma_op, ctx.ep, ctx.opts.transfer_size, &ctx.remote, NULL, enableCompletion);
}

int libefa::Node::rma()
{
    int ret;
    ret = postRma();
    if (ret)
        return ret;
    return getTxCompletion();
}

int libefa::Node::exchangeKeys()
{
    return ft_exchange_keys(&ctx, &ctx.remote);
}

int libefa::Node::initRmaOp(std::string operation)
{
    return ft_parse_rma_opts(&ctx, 'o', strdup(operation.c_str()), ctx.hints, &ctx.opts);
}

void printFabricInfoShort(fi_info *fi)
{
    struct fi_info *cur;
    for (cur = fi; cur; cur = cur->next)
    {
        printf("provider: %s\n", cur->fabric_attr->prov_name);
        printf("    fabric: %s\n", cur->fabric_attr->name),
            printf("    domain: %s\n", cur->domain_attr->name),
            printf("    version: %d.%d\n", FI_MAJOR(cur->fabric_attr->prov_version),
                   FI_MINOR(cur->fabric_attr->prov_version));
        printf("    type: %s\n", fi_tostr(&cur->ep_attr->type, FI_TYPE_EP_TYPE));
        printf("    protocol: %s\n", fi_tostr(&cur->ep_attr->protocol, FI_TYPE_PROTOCOL));
    }
}

void printFabricInfoLong(fi_info *fi)
{
    struct fi_info *cur;
    for (cur = fi; cur; cur = cur->next)
    {
        printf("\n\n---\n");
        printf("%s", fi_tostr(cur, FI_TYPE_INFO));
    }
}

int libefa::Node::printFabricInfo()
{
    int ret = ft_getinfo(&ctx, ctx.hints, &ctx.fi);
    if (ret)
        return ret;

    printFabricInfoShort(ctx.fi);
    return EXIT_SUCCESS;
}