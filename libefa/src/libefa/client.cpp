#include "client.h"
using namespace std;

static int fabric_getaddrinfo(char *name, uint16_t port, struct addrinfo **results)
{
    DEBUG("fabric_getaddrinfo\n");
    int ret;
    const char *err_msg;
    char port_s[6];

    struct addrinfo hints = {
        .ai_flags = AI_NUMERICSERV, /* numeric port is used */
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM, /* TCP socket */
        .ai_protocol = IPPROTO_TCP  /* Any protocol */
    };

    snprintf(port_s, 6, "%" PRIu16, port);

    ret = getaddrinfo(name, port_s, &hints, results);

    if (ret != 0)
    {
        err_msg = gai_strerror(ret);
        DEBUG(err_msg);
        ret = -EXIT_FAILURE;
        return ret;
    }

    return ret;
}

static void print_addrinfo(struct addrinfo *ai, std::string msg)
{
    char s[80] = {0};
    void *addr;

    addr = &((struct sockaddr_in *)ai->ai_addr)->sin_addr;

    inet_ntop(ai->ai_family, addr, s, 80);
    std::cout << msg << " " << s << std::endl;
}

static int ctrl_init_client(struct ctx_connection *ct)
{
    struct addrinfo *results;
    struct addrinfo *rp;
    int errno_save = 0;
    int ret;

    ret = fabric_getaddrinfo(ct->dst_addr, ct->dst_port, &results);
    if (ret)
        return ret;

    if (!results)
    {
        DEBUG("getaddrinfo returned NULL list");
        return -EXIT_FAILURE;
    }

    for (rp = results; rp; rp = rp->ai_next)
    {
        ct->ctrl_connfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (ct->ctrl_connfd == -1)
        {
            errno_save = -1;
            continue;
        }

        struct sockaddr_in in_addr = {0};

        in_addr.sin_family = AF_INET;
        in_addr.sin_port = htons(ct->src_port);
        in_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        ret = bind(ct->ctrl_connfd, (struct sockaddr *)&in_addr, sizeof(in_addr));
        if (ret == -1)
        {
            errno_save = -1;
            close(ct->ctrl_connfd);
            continue;
        }

        print_addrinfo(rp, "CLIENT: connecting to");

        ret = connect(ct->ctrl_connfd, rp->ai_addr, rp->ai_addrlen);
        if (ret != -1)
            break;

        errno_save = -1;
        close(ct->ctrl_connfd);
    }

    if (!rp || ret == -1)
    {
        ret = -errno_save;
        ct->ctrl_connfd = -1;
        DEBUG("Failed to connect\n");
    }
    else
    {
        DEBUG("CLIENT: connected\n");
    }

    freeaddrinfo(results);

    return ret;
}

static int init_fabric_client(struct ctx_connection *ct)
{
    int ret;
    ret = ctrl_init_client(ct);

    if (ret)
        return ret;

    DEBUG("Initializing fabric\n");

    ret = recv_name(ct);
    if (ret < 0)
        return ret;

    DEBUG("CLIENT: getinfo\n");
    ret = fabric_getinfo(ct, ct->hints, &(ct->fi));
    if (ret)
        return ret;

    DEBUG("CLIENT: open fabric resources\n");
    ret = open_fabric_res(ct);
    if (ret)
        return ret;

    DEBUG("CLIENT: allocate active resource\n");
    ret = alloc_active_res(ct, ct->fi);
    if (ret)
        return ret;

    DEBUG("CLIENT: initialize endpoint\n");
    ret = init_ep(ct);
    if (ret)
        return ret;

    ret = send_name(ct, &ct->ep->fid);

    ret = av_insert(ct->av, ct->rem_name, 1, &(ct->remote_fi_addr), 0,
                    NULL);
    if (ret)
        return ret;
    if (ct->fi->domain_attr->caps & FI_LOCAL_COMM)
        ret = av_insert(ct->av, ct->local_name, 1,
                        &(ct->local_fi_addr), 0, NULL);

    if (ret)
        return ret;

    printf("Fabric Initialized\n\n");

    return 0;
}

static int ctrl_sync_client(struct ctx_connection *ct)
{
    int ret;

    snprintf(ct->ctrl_buf, sizeof(MSG_SYNC_Q), "%s",
             MSG_SYNC_Q);

    DEBUG("CLIENT: syncing\n");
    ret = pp_ctrl_send(ct, ct->ctrl_buf, sizeof(MSG_SYNC_Q));

    if (ret < 0)
        return ret;
    if (ret < sizeof(MSG_SYNC_Q))
    {
        printf("CLIENT: bad length of sent data (len=%d/%zu)",
               ret, sizeof(MSG_SYNC_Q));
        return -EBADMSG;
    }
    DEBUG("CLIENT: syncing now\n");

    ret = pp_ctrl_recv_str(ct, ct->ctrl_buf, sizeof(MSG_SYNC_A));

    if (ret < 0)
        return ret;
    if (strcmp(ct->ctrl_buf, MSG_SYNC_A))
    {
        printf("CLIENT: sync error while acking A: <%s> "
               "(len=%zu)\n",
               ct->ctrl_buf, strlen(ct->ctrl_buf));
        return -EBADMSG;
    }
    DEBUG("CLIENT: synced\n");

    return 0;
}

static int ctrl_txrx_msg_count_client(struct ctx_connection *ct)
{
    int ret;

    memset(&ct->ctrl_buf, '\0', MSG_LEN_CNT + 1);
    snprintf(ct->ctrl_buf, MSG_LEN_CNT + 1, "%ld", ct->cnt_ack_msg);

    // if (FLAGS_debug)
        printf("CLIENT: sending count = <%s> (len=%zu)\n", ct->ctrl_buf, strlen(ct->ctrl_buf));

    ret = pp_ctrl_send(ct, ct->ctrl_buf, MSG_LEN_CNT);
    if (ret < 0)
        return ret;
    if (ret < MSG_LEN_CNT)
    {
        printf("CLIENT: bad length of sent data (len=%d/%d)", ret, MSG_LEN_CNT);
        return -EBADMSG;
    }
    DEBUG("CLIENT: sent count\n");

    ret = pp_ctrl_recv_str(ct, ct->ctrl_buf,
                           sizeof(MSG_CHECK_CNT_OK));
    if (ret < 0)
        return ret;
    if (ret < sizeof(MSG_CHECK_CNT_OK))
    {
        printf("CLIENT: bad length of received data (len=%d/%zu)", ret, sizeof(MSG_CHECK_CNT_OK));
        return -EBADMSG;
    }

    if (strcmp(ct->ctrl_buf, MSG_CHECK_CNT_OK))
    {
        printf("CLIENT: error while server acking the count: "
               "<%s> (len=%zu)\n",
               ct->ctrl_buf, strlen(ct->ctrl_buf));
        return ret;
    }
    DEBUG("CLIENT: count acked by server\n");

    return 0;
}

static int init_data_transfer_client(struct ctx_connection *ct)
{
    int ret, i;

    banner_fabric_info(ct);

    ret = ctrl_sync_client(ct);
    if (ret)
        return ret;

    DEBUG("CLIENT: Starting data transfer\n");
    chrono_start(ct);

    for (i = 0; i < ct->iterations; i++)
    {
        if (ct->transfer_size < ct->fi->tx_attr->inject_size)
            ret = pp_inject(ct, ct->ep, ct->transfer_size);
        else
            ret = pp_tx(ct, ct->ep, ct->transfer_size);
        if (ret)
            return ret;

        ret = pp_rx(ct, ct->ep, ct->transfer_size);
        if (ret)
            return ret;
    }

    chrono_stop(ct);
    DEBUG("CLIENT: Completed data transfer\n");

    ret = ctrl_txrx_msg_count_client(ct);
    if (ret)
        return ret;

    show_perf(NULL, ct->transfer_size, ct->iterations,
              ct->cnt_ack_msg, ct->start, ct->end, 2);

    return ret;
}

static int run_dgram_client(struct ctx_connection *ct)
{
    int ret, i;

    DEBUG("Selected endpoint: DGRAM\n");

    ret = init_fabric_client(ct);
    if (ret)
        return ret;

    ret = init_data_transfer_client(ct);
    if (ret)
        return ret;

    return ret;
}

void start_client()
{
    printf("Starting client...\n\n");

    struct ctx_connection ct = {};

    // ct.dst_addr = const_cast<char *>(FLAGS_dst_addr.c_str());
    // ct.dst_port = FLAGS_dst_port;
    // ct.iterations = FLAGS_iterations;
    // ct.transfer_size = FLAGS_payload_size;

    ct.hints = fi_allocinfo();
    generate_hints(&(ct.hints));

    run_dgram_client(&ct);
}