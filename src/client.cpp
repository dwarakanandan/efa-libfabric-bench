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
    ret = EXIT_SUCCESS;
}

static void print_addrinfo(struct addrinfo *ai, std::string msg)
{
	char s[80] = {0};
	void *addr;

	addr = &((struct sockaddr_in *)ai->ai_addr)->sin_addr;

	inet_ntop(ai->ai_family, addr, s, 80);
    std::cout << msg << "  " << s << std::endl;
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

    DEBUG("Connection-less endpoint: initializing address vector\n");

    DEBUG("CLIENT: getinfo\n");
    ret = fabric_getinfo(ct, ct->hints, &(ct->fi));

    if (ret)
        return ret;
}

static int run_ping_dgram_client(struct ctx_connection *ct)
{
    int ret;

    DEBUG("Selected endpoint: DGRAM\n");

    ret = init_fabric_client(ct);
    if (ret)
        return ret;
}

void start_client()
{
    DEBUG("Starting client\n");

    struct ctx_connection ct = {};

    ct.dst_addr = const_cast<char *>(FLAGS_dst_addr.c_str());
    ct.dst_port = FLAGS_dst_port;

    ct.hints = fi_allocinfo();
    ct.hints->fabric_attr->prov_name = const_cast<char *>(FLAGS_provider.c_str());
    ct.hints->ep_attr->type = FI_EP_DGRAM;
    ct.hints->caps = FI_MSG;
    ct.hints->mode = FI_CONTEXT | FI_CONTEXT2 | FI_MSG_PREFIX;
    ct.hints->domain_attr->mr_mode = FI_MR_LOCAL;

    run_ping_dgram_client(&ct);
}