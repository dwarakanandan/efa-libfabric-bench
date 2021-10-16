#include "common.h"

using namespace std;

void DEBUG(std::string str)
{
    if (FLAGS_debug)
    {
        std::cout << str << std::endl;
    }
}

int fabric_getinfo(struct ctx_connection *ct, struct fi_info *hints, struct fi_info **info)
{
    int ret;
    uint64_t flags = 0;

    ret = fi_getinfo(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION),
                     NULL, NULL, flags, hints, info);

    if (ret)
    {
        PRINTERR("fi_getinfo", ret);
        return ret;
    }

    if (((*info)->tx_attr->mode & FI_CONTEXT2) != 0)
    {
        ct->tx_ctx_ptr = &(ct->tx_ctx[0]);
    }
    else if (((*info)->tx_attr->mode & FI_CONTEXT) != 0)
    {
        ct->tx_ctx_ptr = &(ct->tx_ctx[1]);
    }
    else if (((*info)->mode & FI_CONTEXT2) != 0)
    {
        ct->tx_ctx_ptr = &(ct->tx_ctx[0]);
    }
    else if (((*info)->mode & FI_CONTEXT) != 0)
    {
        ct->tx_ctx_ptr = &(ct->tx_ctx[1]);
    }
    else
    {
        ct->tx_ctx_ptr = NULL;
    }

    if (((*info)->rx_attr->mode & FI_CONTEXT2) != 0)
    {
        ct->rx_ctx_ptr = &(ct->rx_ctx[0]);
    }
    else if (((*info)->rx_attr->mode & FI_CONTEXT) != 0)
    {
        ct->rx_ctx_ptr = &(ct->rx_ctx[1]);
    }
    else if (((*info)->mode & FI_CONTEXT2) != 0)
    {
        ct->rx_ctx_ptr = &(ct->rx_ctx[0]);
    }
    else if (((*info)->mode & FI_CONTEXT) != 0)
    {
        ct->rx_ctx_ptr = &(ct->rx_ctx[1]);
    }
    else
    {
        ct->rx_ctx_ptr = NULL;
    }

    if ((hints->caps & FI_DIRECTED_RECV) == 0)
    {
        (*info)->caps &= ~FI_DIRECTED_RECV;
        (*info)->rx_attr->caps &= ~FI_DIRECTED_RECV;
    }

    print_short_info(*info);

    return 0;
}

void print_long_info(struct fi_info *info)
{
    struct fi_info *cur;
    for (cur = info; cur; cur = cur->next)
    {
        printf("\n\n---\n");
        printf("%s", fi_tostr(cur, FI_TYPE_INFO));
    }
}

void print_short_info(struct fi_info *info)
{
    struct fi_info *cur;
    for (cur = info; cur; cur = cur->next)
    {
        printf("provider: %s\n", cur->fabric_attr->prov_name);
        printf("    fabric: %s\n", cur->fabric_attr->name),
            printf("    domain: %s\n", cur->domain_attr->name),
            printf("    version: %d.%d\n", FI_MAJOR(cur->fabric_attr->prov_version),
                   FI_MINOR(cur->fabric_attr->prov_version));
        printf("    type: %s\n", fi_tostr(&cur->ep_attr->type, FI_TYPE_EP_TYPE));
        printf("    protocol: %s\n", fi_tostr(&cur->ep_attr->protocol, FI_TYPE_PROTOCOL));
    }
}