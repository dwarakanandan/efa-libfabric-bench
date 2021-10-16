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

    DEBUG("SERVER: waiting for connection\n");

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

    DEBUG("Connection-less endpoint: initializing address vector\n");

    DEBUG("SERVER: getinfo\n");
    ret = fabric_getinfo(ct, ct->hints, &(ct->fi));

    if (ret)
        return ret;
}

static int run_ping_dgram_server(struct ctx_connection *ct)
{
    int ret;

    DEBUG("Selected endpoint: DGRAM\n");

    ret = init_fabric_server(ct);
    if (ret)
        return ret;
}

void start_server()
{
    DEBUG("Starting server\n");

    struct ctx_connection ct = {};

    ct.src_port = FLAGS_src_port;

    ct.hints = fi_allocinfo();
    ct.hints->fabric_attr->prov_name = const_cast<char *>(FLAGS_provider.c_str());
    ct.hints->ep_attr->type = FI_EP_DGRAM;
    ct.hints->caps = FI_MSG;
    ct.hints->mode = FI_CONTEXT | FI_CONTEXT2 | FI_MSG_PREFIX;
    ct.hints->domain_attr->mr_mode = FI_MR_LOCAL;

    run_ping_dgram_server(&ct);
}