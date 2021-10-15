#include "fi_info.h"

void print_short_info(struct fi_info *info)
{
    struct fi_info *cur;
    for (cur = info; cur ; cur = cur->next)
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

void print_fi_info()
{
    int ret;
    uint64_t flags = 0;

    struct fi_info *hints, *info;
    hints = fi_allocinfo();
    char provider[] = "sockets";
    hints->fabric_attr->prov_name = provider;
    hints->ep_attr->type = FI_EP_DGRAM;
    hints->caps = FI_MSG;
    hints->mode = FI_CONTEXT | FI_CONTEXT2 | FI_MSG_PREFIX;
    hints->domain_attr->mr_mode = FI_MR_LOCAL;

    ret = fi_getinfo(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION),
                     NULL, NULL, flags, hints, &info);

    if (ret)
    {
        PRINTERR("fi_getinfo", ret);
    }
    else
    {
        print_short_info(info);
    }
}