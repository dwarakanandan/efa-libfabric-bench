#include "server.h"

static int ctrl_init_server(struct ctx_connection *ct)
{
    int optval = 1;
    SOCKET listenfd;
    int ret;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        close(listenfd);
        return -1;
    }

    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                     (const char *)&optval, sizeof(optval));
    if (ret == -1)
    {
        close(listenfd);
        return -1;
    }

    struct sockaddr_in ctrl_addr = {0};

    ctrl_addr.sin_family = AF_INET;
    ctrl_addr.sin_port = htons(ct->src_port);
    ctrl_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(listenfd, (struct sockaddr *)&ctrl_addr,
               sizeof(ctrl_addr));
    if (ret == -1)
    {
        close(listenfd);
        return -1;
    }

    ret = listen(listenfd, 10);
    if (ret == -1)
    {
        close(listenfd);
        return -1;
    }

    printf("SERVER: waiting for connection\n");

    ct->ctrl_connfd = accept(listenfd, NULL, NULL);
    if (ct->ctrl_connfd == -1)
    {
        close(ct->ctrl_connfd);
        return -1;
    }

    close(listenfd);

    DEBUG("SERVER: connected\n");
    struct timeval tv = {
        .tv_sec = 5};
    ret = setsockopt(ct->ctrl_connfd, SOL_SOCKET, SO_RCVTIMEO,
                     (const char *)&tv, sizeof(struct timeval));
    if (ret == -1)
    {
        return -1;
    }

    DEBUG("Control messages initialized\n");

    return ret;
}

static int init_fabric_server(struct ctx_connection *ct)
{
    int ret;
    ret = ctrl_init_server(ct);

    if (ret)
        return ret;

    DEBUG("Initializing fabric\n");

    DEBUG("SERVER: getinfo\n");
    ret = fabric_getinfo(ct, ct->hints, &(ct->fi));
    if (ret)
        return ret;

    DEBUG("SERVER: open fabric resources\n");
    ret = open_fabric_res(ct);
    if (ret)
        return ret;

    DEBUG("SERVER: allocate active resource\n");
    ret = alloc_active_res(ct, ct->fi);
    if (ret)
        return ret;

    DEBUG("SERVER: initialize endpoint\n");
    ret = init_ep(ct);
    if (ret)
        return ret;

    ret = send_name(ct, &ct->ep->fid);
    if (ret < 0)
        return ret;

    ret = recv_name(ct);
    if (ret < 0)
        return ret;

    if (ct->fi->domain_attr->caps & FI_LOCAL_COMM)
    {
        ret = av_insert(ct->av, ct->local_name, 1,
                        &(ct->local_fi_addr), 0, NULL);
        if (ret)
            return ret;
    }
    ret = av_insert(ct->av, ct->rem_name, 1, &(ct->remote_fi_addr), 0,
                    NULL);
    if (ret)
        return ret;

    printf("Fabric Initialized\n\n");

    return ret;
}

static int ctrl_sync_server(struct ctx_connection *ct)
{
    int ret;

    DEBUG("SERVER: syncing\n");
    ret = pp_ctrl_recv_str(ct, ct->ctrl_buf, sizeof(MSG_SYNC_Q));

    if (ret < 0)
        return ret;
    if (strcmp(ct->ctrl_buf, MSG_SYNC_Q))
    {
        printf("SERVER: sync error while acking Q: <%s> "
               "(len=%zu)\n",
               ct->ctrl_buf, strlen(ct->ctrl_buf));
        return -EBADMSG;
    }

    DEBUG("SERVER: syncing now\n");
    snprintf(ct->ctrl_buf, sizeof(MSG_SYNC_A), "%s", MSG_SYNC_A);

    ret = pp_ctrl_send(ct, ct->ctrl_buf, sizeof(MSG_SYNC_A));
    if (ret < 0)
        return ret;
    if (ret < sizeof(MSG_SYNC_A))
    {
        printf("SERVER: bad length of sent data (len=%d/%zu)", ret, sizeof(MSG_SYNC_A));
        return -EBADMSG;
    }
    DEBUG("SERVER: synced\n");

    return 0;
}

static int ctrl_txrx_msg_count_server(struct ctx_connection *ct)
{
    int ret;

    DEBUG("Exchanging ack count\n");

    memset(&ct->ctrl_buf, '\0', MSG_LEN_CNT + 1);

    DEBUG("SERVER: receiving count\n");
    ret = pp_ctrl_recv(ct, ct->ctrl_buf, MSG_LEN_CNT);
    if (ret < 0)
        return ret;
    if (ret < MSG_LEN_CNT)
    {
        printf("SERVER: bad length of received data (len=%d/%d)", ret, MSG_LEN_CNT);
        return -EBADMSG;
    }
    ct->cnt_ack_msg = parse_ulong(ct->ctrl_buf, -1);
    if (ct->cnt_ack_msg < 0)
        return ret;

    if (FLAGS_debug)
        printf("SERVER: received count = <%ld> (len=%zu)\n", ct->cnt_ack_msg, strlen(ct->ctrl_buf));

    snprintf(ct->ctrl_buf, sizeof(MSG_CHECK_CNT_OK), "%s", MSG_CHECK_CNT_OK);
    ret = pp_ctrl_send(ct, ct->ctrl_buf, sizeof(MSG_CHECK_CNT_OK));
    if (ret < 0)
        return ret;
    if (ret < sizeof(MSG_CHECK_CNT_OK))
    {
        printf("CLIENT: bad length of received data (len=%d/%zu)", ret, sizeof(MSG_CHECK_CNT_OK));
        return -EBADMSG;
    }
    DEBUG("SERVER: acked count to client\n");

    return 0;
}

static int init_data_transfer_server(struct ctx_connection *ct)
{
    int ret, i;

    banner_fabric_info(ct);

    ret = ctrl_sync_server(ct);
    if (ret)
        return ret;

    DEBUG("SERVER: Starting data transfer\n");
    chrono_start(ct);

    for (i = 0; i < ct->iterations; i++)
    {
        ret = pp_rx(ct, ct->ep, ct->transfer_size);
        if (ret)
            return ret;

        if (ct->transfer_size < ct->fi->tx_attr->inject_size)
            ret = pp_inject(ct, ct->ep,
                            ct->transfer_size);
        else
            ret = pp_tx(ct, ct->ep, ct->transfer_size);
        if (ret)
            return ret;
    }

    chrono_stop(ct);
    DEBUG("SERVER: Completed data transfer\n");

    ret = ctrl_txrx_msg_count_server(ct);
    if (ret)
        return ret;

    show_perf(NULL, ct->transfer_size, ct->iterations,
              ct->cnt_ack_msg, ct->start, ct->end, 2);

    return ret;
}

static int run_dgram_server(struct ctx_connection *ct)
{
    int ret;

    DEBUG("Selected endpoint: DGRAM\n");

    ret = init_fabric_server(ct);
    if (ret)
        return ret;

    ret = init_data_transfer_server(ct);
    if (ret)
        return ret;

    return ret;
}

void start_server()
{
    printf("Starting server...\n\n");

    struct ctx_connection ct = {};

    ct.src_port = FLAGS_src_port;
    ct.iterations = FLAGS_iterations;
    ct.transfer_size = FLAGS_payload_size;

    ct.hints = fi_allocinfo();
    generate_hints(&(ct.hints));

    run_dgram_server(&ct);
}