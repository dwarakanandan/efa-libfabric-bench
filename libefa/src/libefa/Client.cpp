#include "Client.h"

libefa::Client::Client(std::string provider, std::string endpoint, std::string destinationAddress, uint16_t destinationPort)
{
    this->provider = provider;
    this->endpoint = endpoint;
    this->destinationAddress = destinationAddress;
    this->destinationPort = destinationPort;

    ctx = ConnectionContext();
    ctx.dst_addr = const_cast<char *>(this->destinationAddress.c_str());
    ctx.dst_port = destinationPort;
}

int libefa::Client::fabricGetaddrinfo(struct addrinfo **results)
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

    snprintf(port_s, 6, "%" PRIu16, ctx.dst_port);

    ret = getaddrinfo(ctx.dst_addr, port_s, &hints, results);

    if (ret != 0)
    {
        err_msg = gai_strerror(ret);
        DEBUG(err_msg);
        ret = -EXIT_FAILURE;
        return ret;
    }

    return ret;
}

int libefa::Client::ctrlInit()
{
    struct addrinfo *results;
    struct addrinfo *rp;
    int errno_save = 0;
    int ret;

    ret = fabricGetaddrinfo(&results);
    if (ret)
        return ret;

    if (!results)
    {
        DEBUG("getaddrinfo returned NULL list");
        return -EXIT_FAILURE;
    }

    for (rp = results; rp; rp = rp->ai_next)
    {
        ctx.ctrl_connfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (ctx.ctrl_connfd == -1)
        {
            errno_save = -1;
            continue;
        }

        struct sockaddr_in in_addr = {0};

        in_addr.sin_family = AF_INET;
        in_addr.sin_port = htons(ctx.src_port);
        in_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        ret = bind(ctx.ctrl_connfd, (struct sockaddr *)&in_addr, sizeof(in_addr));
        if (ret == -1)
        {
            errno_save = -1;
            close(ctx.ctrl_connfd);
            continue;
        }

        FabricUtil::prinAddrinfo(rp, "CLIENT: connecting to");

        ret = connect(ctx.ctrl_connfd, rp->ai_addr, rp->ai_addrlen);
        if (ret != -1)
            break;

        errno_save = -1;
        close(ctx.ctrl_connfd);
    }

    if (!rp || ret == -1)
    {
        ret = -errno_save;
        ctx.ctrl_connfd = -1;
        DEBUG("Failed to connect\n");
    }
    else
    {
        DEBUG("CLIENT: connected\n");
    }

    freeaddrinfo(results);

    return ret;
}

int libefa::Client::initFabric()
{
    int ret;
    ret = ctrlInit();

    if (ret)
        return ret;

    DEBUG("Initializing fabric\n");

    ret = FabricUtil::receiveName(&ctx);
    if (ret < 0)
        return ret;

    DEBUG("CLIENT: getinfo\n");
    ret = ctx.fi.initFabricInfo(provider, endpoint);
    if (ret)
        return ret;

    DEBUG("CLIENT: open fabric resources\n");
    ret = FabricUtil::openFabricRes(&ctx);
    if (ret)
        return ret;

    DEBUG("CLIENT: allocate active resource\n");
    ret = FabricUtil::allocActiveRes(&ctx);
    if (ret)
        return ret;

    DEBUG("CLIENT: initialize endpoint\n");
    ret = FabricUtil::initEndpoint(&ctx);
    if (ret)
        return ret;

    ret = FabricUtil::sendName(&ctx, &(ctx.ep->fid));

    ret = FabricUtil::insertAddressVector(ctx.av, ctx.rem_name, 1, &(ctx.remote_fi_addr), 0, NULL);
    if (ret)
        return ret;
    if (ctx.fi.info->domain_attr->caps & FI_LOCAL_COMM)
        ret = FabricUtil::insertAddressVector(ctx.av, ctx.local_name, 1, &(ctx.local_fi_addr), 0, NULL);

    if (ret)
        return ret;

    printf("*** Fabric Initialized ***\n\n");

    ctx.fi.printFabricInfoBanner();

    return 0;
}

int libefa::Client::ctrlSync()
{
    int ret;

    snprintf(ctx.ctrl_buf, sizeof(MSG_SYNC_Q), "%s",
             MSG_SYNC_Q);

    DEBUG("CLIENT: syncing\n");
    ret = FabricUtil::ctrlSend(&ctx, ctx.ctrl_buf, sizeof(MSG_SYNC_Q));

    if (ret < 0)
        return ret;
    if (ret < sizeof(MSG_SYNC_Q))
    {
        printf("CLIENT: bad length of sent data (len=%d/%zu)",
               ret, sizeof(MSG_SYNC_Q));
        return -EBADMSG;
    }
    DEBUG("CLIENT: syncing now\n");

    ret = FabricUtil::ctrlReceiveString(&ctx, ctx.ctrl_buf, sizeof(MSG_SYNC_A));

    if (ret < 0)
        return ret;
    if (strcmp(ctx.ctrl_buf, MSG_SYNC_A))
    {
        printf("CLIENT: sync error while acking A: <%s> "
               "(len=%zu)\n",
               ctx.ctrl_buf, strlen(ctx.ctrl_buf));
        return -EBADMSG;
    }
    DEBUG("CLIENT: synced\n");

    return 0;
}

int libefa::Client::init()
{
    int ret;

    if (endpoint == "rdm")
    {
        DEBUG("CLIENT: Selected endpoint: RDM\n");
    }
    else
    {
        DEBUG("CLIENT: Selected endpoint: DGRAM\n");
    }

    ret = initFabric();
    if (ret)
        return ret;

    ret = ctrlSync();

    return ret;
}

libefa::ConnectionContext libefa::Client::getConnectionContext()
{
    return this->ctx;
}