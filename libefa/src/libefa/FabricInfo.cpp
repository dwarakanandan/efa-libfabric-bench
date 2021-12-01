#include "FabricInfo.h"

int libefa::FabricInfo::initFabricInfo(std::string provider, std::string endpoint, bool isTagged)
{
    int ret;
    uint64_t flags = 0;

    hints->fabric_attr->prov_name = const_cast<char *>(provider.c_str());
    hints->ep_attr->type = (endpoint == "rdm") ? FI_EP_RDM : FI_EP_DGRAM;

    // MSG_PREFIX is the only fabric mode supported by EFA
    hints->mode = FI_MSG_PREFIX;

    // DGRAM only supports FI_MSG, only if we are running RDM, set caps to support Tagged packets
    if ((hints->ep_attr->type == FI_EP_RDM) && isTagged)
    {
        hints->caps = FI_TAGGED;
    }
    else
    {
        hints->caps = FI_MSG;
    }

    hints->domain_attr->mode = ~0;
    hints->domain_attr->mr_mode = FI_MR_LOCAL | FI_MR_VIRT_ADDR | FI_MR_ALLOCATED | FI_MR_PROV_KEY;
    // hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    // hints->domain_attr->threading = FI_THREAD_DOMAIN;
    // hints->tx_attr->msg_order = FI_ORDER_SAS;
    // hints->rx_attr->msg_order = FI_ORDER_SAS;

    ret = fi_getinfo(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION),
                     NULL, NULL, flags, hints, &info);

    if (ret)
    {
        PRINTERR("fi_getinfo", ret);
        return ret;
    }

    if (info->caps & FI_TAGGED)
    {
        DEBUG("Tagged message transfer\n");
    }
    else
    {
        DEBUG("Untagged message transfer\n");
    }

    return EXIT_SUCCESS;
}

int libefa::FabricInfo::initFabricInfo(std::string provider, struct fi_info *h)
{
    int ret;
    uint64_t flags = 0;

    hints = h;

    ret = fi_getinfo(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION),
                     NULL, NULL, flags, hints, &info);

    if (ret)
    {
        PRINTERR("fi_getinfo", ret);
        return ret;
    }

    return EXIT_SUCCESS;
}

void libefa::FabricInfo::printFabricInfoShort()
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

void libefa::FabricInfo::printFabricInfoLong()
{
    struct fi_info *cur;
    for (cur = info; cur; cur = cur->next)
    {
        printf("\n\n---\n");
        printf("%s", fi_tostr(cur, FI_TYPE_INFO));
    }
}

void libefa::FabricInfo::printFabricInfoBanner()
{
    struct fi_info *cur = info;
    printf("Fabric initialized with fi_info:\n\n");
    printf("provider: %s\n", cur->fabric_attr->prov_name);
    printf("    fabric: %s\n", cur->fabric_attr->name),
        printf("    domain: %s\n", cur->domain_attr->name),
        printf("    version: %d.%d\n", FI_MAJOR(cur->fabric_attr->prov_version),
               FI_MINOR(cur->fabric_attr->prov_version));
    printf("    type: %s\n", fi_tostr(&cur->ep_attr->type, FI_TYPE_EP_TYPE));
    printf("    protocol: %s\n", fi_tostr(&cur->ep_attr->protocol, FI_TYPE_PROTOCOL));
    std::cout << std::endl
              << std::endl;
}
