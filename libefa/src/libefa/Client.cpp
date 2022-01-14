#include "Client.h"

libefa::Client::Client(std::string provider, std::string endpoint, std::string port, fi_info *userHints, std::string destinationAddress)
    : Node(provider, endpoint, userHints)
{
    ctx.opts.dst_addr = strdup(destinationAddress.c_str());
    ctx.opts.dst_port = strdup(port.c_str());
}