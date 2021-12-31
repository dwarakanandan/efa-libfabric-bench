#include "Node.h"

libefa::Node::Node(std::string provider, std::string endpoint, fi_info *userHints)
{
    hints = userHints;
    init_opts(&opts);

    opts.options |= FT_OPT_BW;

    hints->addr_format = opts.address_format;
    hints->domain_attr->mr_mode = opts.mr_mode;

    ft_parseinfo('p', strdup(provider.c_str()), hints, &opts);
    ft_parseinfo('e', strdup(endpoint.c_str()), hints, &opts);

    // Enable out-of-band address exchange for the EFA provider
    if (provider == "efa")
    {
        opts.options |= FT_OPT_OOB_SYNC;
        opts.options |= FT_OPT_OOB_ADDR_EXCH;
    }
}

int libefa::Node::init()
{
    return ft_init_fabric();
}

int libefa::Node::sync()
{
    return ft_sync();
}

libefa::Node::~Node()
{
    ft_free_res();
}

int libefa::Node::initTxBuffer(size_t size)
{
    opts.transfer_size = size;
    return ft_fill_buf((char *)tx_buf + ft_tx_prefix_size(),  opts.transfer_size);
}

int libefa::Node::postTx()
{
    return ft_post_tx(ep, remote_fi_addr,  opts.transfer_size, NO_CQ_DATA, &tx_ctx);
}

int libefa::Node::getTxCompletion()
{
    return ft_get_tx_comp(tx_seq);
}

int libefa::Node::getNTxCompletion(int n)
{
    return ft_get_tx_comp(tx_cq_cntr + n);
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
    return ft_inject(ep, remote_fi_addr,  opts.transfer_size);
}

int libefa::Node::rx()
{
    int ret;
    ret = ft_get_rx_comp(rx_seq);
    if (ret)
        return ret;
    return ft_post_rx(ep, rx_size, &rx_ctx);
}

void libefa::Node::startTimer()
{
    ft_start();
}

void libefa::Node::stopTimer()
{
    ft_stop();
}

void libefa::Node::showTransferStatistics(int iterations, int transfersPerIterations)
{
    show_perf(NULL,  opts.transfer_size, iterations, &start, &end, transfersPerIterations);
}

int libefa::Node::postRma()
{
    return ft_post_rma(opts.rma_op, ep,  opts.transfer_size, &remote, NULL);
}

int libefa::Node::postRmaSelectiveComp(bool enableCompletion)
{
    return ft_post_rma_selective_comp(opts.rma_op, ep,  opts.transfer_size, &remote, NULL, enableCompletion);
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
    return ft_exchange_keys(&remote);
}

int libefa::Node::initRmaOp(std::string operation)
{
    return ft_parse_rma_opts('o', strdup(operation.c_str()), hints, &opts);
}

void printFabricInfoShort()
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

void printFabricInfoLong()
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
    int	ret = ft_getinfo(hints, &fi);
	if (ret)
		return ret;
    
    printFabricInfoShort();
    return EXIT_SUCCESS;
}