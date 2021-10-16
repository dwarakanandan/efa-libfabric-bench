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

    printf("Fabric Initialized\n");

    return 0;
}

static int run_dgram_client(struct ctx_connection *ct)
{
    int ret;

    DEBUG("Selected endpoint: DGRAM\n");

    ret = init_fabric_client(ct);
    if (ret)
        return ret;

    return ret;
}

void start_client()
{
    printf("Starting client\n");

    struct ctx_connection ct = {};

    ct.dst_addr = const_cast<char *>(FLAGS_dst_addr.c_str());
    ct.dst_port = FLAGS_dst_port;

    ct.hints = fi_allocinfo();
    generate_hints(&(ct.hints));

    run_dgram_client(&ct);
}