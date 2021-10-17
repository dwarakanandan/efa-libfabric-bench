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

#define CTRL_BUF_LEN 64
#define MSG_SYNC_Q "q"
#define MSG_SYNC_A "a"

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

    char ctrl_buf[CTRL_BUF_LEN + 1];

    uint64_t start, end;

    int iterations;
	int transfer_size;

    long cnt_ack_msg;
};