#include "Node.h"

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
    this->txPayloadSize = size;
    return ft_fill_buf((char *)tx_buf + ft_tx_prefix_size(), this->txPayloadSize);
}

int libefa::Node::postTx()
{
    return ft_post_tx(ep, remote_fi_addr, this->txPayloadSize, NO_CQ_DATA, &tx_ctx);
}

int libefa::Node::getTxCompletion()
{
    return ft_get_tx_comp(tx_seq);
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
    return ft_inject(ep, remote_fi_addr, this->txPayloadSize);
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
    show_perf(NULL, this->txPayloadSize, iterations, &start, &end, transfersPerIterations);
}

int libefa::Node::postRma()
{
    return ft_post_rma(opts.rma_op, ep, this->txPayloadSize, &remote, NULL);
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
    // Set out of band key exchange
    // ft_parsecsopts('b', NULL, &opts);
    return ft_parse_rma_opts('o', strdup(operation.c_str()), hints, &opts);
}