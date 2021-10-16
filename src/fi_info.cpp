#include "fi_info.h"

void print_fi_info()
{
    int ret;
    uint64_t flags = 0;

    struct fi_info *hints, *info;
    hints = fi_allocinfo();
    hints->fabric_attr->prov_name = const_cast<char *>(FLAGS_provider.c_str());
    hints->ep_attr->type = FI_EP_DGRAM;

    // hints->caps = FI_MSG;
    // hints->mode = FI_CONTEXT | FI_CONTEXT2 | FI_MSG_PREFIX;
    // hints->domain_attr->mr_mode = FI_MR_LOCAL;

    // hints->mode = ~0;
    // hints->domain_attr->mode = ~0;
    // hints->domain_attr->mr_mode = ~(FI_MR_BASIC | FI_MR_SCALABLE);

    ret = fi_getinfo(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION),
                     NULL, NULL, flags, hints, &info);

    if (ret)
    {
        PRINTERR("fi_getinfo", ret);
    }
    else
    {
        print_long_info(info);
    }
}