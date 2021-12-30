#include "Server.h"

libefa::Server::Server(std::string provider, std::string endpoint, fi_info *userHints)
{
    hints = userHints;
    init_opts(&opts);

	opts.options |= FT_OPT_BW;

    hints->mode = FI_CONTEXT;
    hints->addr_format = opts.address_format;
	hints->domain_attr->mr_mode = opts.mr_mode;

    ft_parseinfo('p', strdup(provider.c_str()), hints, &opts);
	ft_parseinfo('e', strdup(endpoint.c_str()), hints, &opts);
}