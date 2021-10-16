#pragma once

#include <iostream>

#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <getopt.h>
#include <inttypes.h>
#include <netdb.h>
#include <poll.h>
#include <limits.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_tagged.h>

#define SOCKET int

#define PP_CTRL_BUF_LEN 64

struct pp_opts
{
    uint16_t src_port;
    uint16_t dst_port;
    char *dst_addr;
    int iterations;
    int transfer_size;
    int sizes_enabled;
    int options;
};

struct ct_pingpong
{
    struct fi_info *fi_pep, *fi, *hints;
    struct fid_fabric *fabric;
    struct fid_domain *domain;
    struct fid_pep *pep;
    struct fid_ep *ep;
    struct fid_cq *txcq, *rxcq;
    struct fid_mr *mr;
    struct fid_av *av;
    struct fid_eq *eq;

    struct fid_mr no_mr;
    void *tx_ctx_ptr, *rx_ctx_ptr;
    struct fi_context tx_ctx[2], rx_ctx[2];
    uint64_t remote_cq_data;

    uint64_t tx_seq, rx_seq, tx_cq_cntr, rx_cq_cntr;

    fi_addr_t local_fi_addr, remote_fi_addr;
    void *buf, *tx_buf, *rx_buf;
    size_t buf_size, tx_size, rx_size;
    size_t rx_prefix_size, tx_prefix_size;

    int timeout_sec;
    uint64_t start, end;

    struct fi_av_attr av_attr;
    struct fi_eq_attr eq_attr;
    struct fi_cq_attr cq_attr;
    struct pp_opts opts;

    long cnt_ack_msg;

    SOCKET ctrl_connfd;
    char ctrl_buf[PP_CTRL_BUF_LEN + 1];

    void *local_name, *rem_name;
};

struct ctx_connection
{
    struct fi_info *fi, *hints;
    void *tx_ctx_ptr, *rx_ctx_ptr;
    struct fi_context tx_ctx[2], rx_ctx[2];

    uint16_t src_port;
    uint16_t dst_port;
    char *dst_addr;
    SOCKET ctrl_connfd;

    struct fid_fabric *fabric;
    struct fid_domain *domain;
    struct fid_eq *eq;
    struct fi_eq_attr eq_attr;

    size_t rx_prefix_size, tx_prefix_size;
    struct fi_cq_attr cq_attr;
    struct fid_cq *txcq, *rxcq;
    struct fi_av_attr av_attr;
    struct fid_ep *ep;
    struct fid_av *av;

    size_t buf_size, tx_size, rx_size;
    void *buf, *tx_buf, *rx_buf;
    uint64_t remote_cq_data;
    struct fid_mr *mr;
    struct fid_mr no_mr;

    int timeout_sec;
    uint64_t tx_seq, rx_seq, tx_cq_cntr, rx_cq_cntr;

    char *local_name, *rem_name;
    struct fi_info *fi_pep;

    fi_addr_t local_fi_addr, remote_fi_addr;
};