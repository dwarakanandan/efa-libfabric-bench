#include "Server.h"

libefa::Server::Server(std::string provider, std::string endpoint, bool isTagged, uint16_t port)
{
    this->provider = provider;
    this->endpoint = endpoint;
    this->isTagged = isTagged;
    this->port = port;

    ctx = ConnectionContext();
    ctx.src_port = this->port;
}

int libefa::Server::ctrlInit()
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
    ctrl_addr.sin_port = htons(port);
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

    ctx.ctrl_connfd = accept(listenfd, NULL, NULL);
    if (ctx.ctrl_connfd == -1)
    {
        close(ctx.ctrl_connfd);
        return -1;
    }

    close(listenfd);

    DEBUG("SERVER: connected\n");
    struct timeval tv = {
        .tv_sec = 5};
    ret = setsockopt(ctx.ctrl_connfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct timeval));
    if (ret == -1)
    {
        return -1;
    }

    DEBUG("Control messages initialized\n");

    return ret;
}

int libefa::Server::initFabric(fi_info *hints)
{
    int ret;
    ret = ctrlInit();

    if (ret)
        return ret;

    DEBUG("Initializing fabric\n");

    DEBUG("SERVER: getinfo\n");
    if (hints != NULL)
    {
        ret = ctx.fi.initFabricInfo(provider, hints);
    }
    else
    {
        ret = ctx.fi.initFabricInfo(provider, endpoint, isTagged);
    }
    if (ret)
        return ret;

    DEBUG("SERVER: open fabric resources\n");
    ret = FabricUtil::openFabricRes(&ctx);
    if (ret)
        return ret;

    DEBUG("SERVER: allocate active resource\n");
    ret = FabricUtil::allocActiveRes(&ctx);
    if (ret)
        return ret;

    DEBUG("SERVER: initialize endpoint\n");
    ret = FabricUtil::initEndpoint(&ctx);
    if (ret)
        return ret;

    ret = FabricUtil::sendName(&ctx, &(ctx.ep->fid));
    if (ret < 0)
        return ret;

    ret = FabricUtil::receiveName(&ctx);
    if (ret < 0)
        return ret;

    if (ctx.fi.info->domain_attr->caps & FI_LOCAL_COMM)
    {
        ret = FabricUtil::insertAddressVector(ctx.av, ctx.local_name, 1, &(ctx.local_fi_addr), 0, NULL);
        if (ret)
            return ret;
    }
    ret = FabricUtil::insertAddressVector(ctx.av, ctx.rem_name, 1, &(ctx.remote_fi_addr), 0, NULL);
    if (ret)
        return ret;

    DEBUG("*** Fabric Initialized ***\n\n");

    if (libefa::ENABLE_DEBUG)
        ctx.fi.printFabricInfoBanner();

    return ret;
}

int libefa::Server::ctrlSync()
{
    int ret;

    DEBUG("SERVER: syncing\n");
    ret = FabricUtil::ctrlReceiveString(&ctx, ctx.ctrl_buf, sizeof(MSG_SYNC_Q));

    if (ret < 0)
        return ret;
    if (strcmp(ctx.ctrl_buf, MSG_SYNC_Q))
    {
        printf("SERVER: sync error while acking Q: <%s> "
               "(len=%zu)\n",
               ctx.ctrl_buf, strlen(ctx.ctrl_buf));
        return -EBADMSG;
    }

    DEBUG("SERVER: syncing now\n");
    snprintf(ctx.ctrl_buf, sizeof(MSG_SYNC_A), "%s", MSG_SYNC_A);

    ret = FabricUtil::ctrlSend(&ctx, ctx.ctrl_buf, sizeof(MSG_SYNC_A));
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

int libefa::Server::init(fi_info *hints)
{
    int ret;

    if (endpoint == "rdm")
    {
        DEBUG("SERVER: Selected endpoint: RDM\n");
    }
    else
    {
        DEBUG("SERVER: Selected endpoint: DGRAM\n");
    }

    ret = initFabric(hints);
    if (ret)
        return ret;

    ret = ctrlSync();

    return ret;
}

libefa::ConnectionContext libefa::Server::getConnectionContext()
{
    return this->ctx;
}

int libefa::Server::exchangeRmaIov()
{
    int ret;
    FabricUtil::ctrlSendRmaIov(&ctx);
    if (ret < 0)
        return ret;

    FabricUtil::ctrlReceiveRmaIov(&ctx);
    if (ret < 0)
        return ret;

    DEBUG("SERVER: RMA IOv exchange done\n");
    return 0;
}