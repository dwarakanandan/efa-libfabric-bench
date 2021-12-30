#include "Client.h"

libefa::Client::Client(std::string provider, std::string endpoint, std::string destinationAddress, fi_info *userHints)
{
    hints = userHints;
    init_opts(&opts);

    opts.options |= FT_OPT_BW;

    hints->mode |= FI_CONTEXT;
    hints->addr_format = opts.address_format;
	hints->domain_attr->mr_mode = opts.mr_mode;

    ft_parseinfo('p', strdup(provider.c_str()), hints, &opts);
    ft_parseinfo('e', strdup(endpoint.c_str()), hints, &opts);
    opts.dst_addr = strdup(destinationAddress.c_str());
}