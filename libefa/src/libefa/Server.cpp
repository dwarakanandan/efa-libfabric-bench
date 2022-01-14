#include "Server.h"

libefa::Server::Server(std::string provider, std::string endpoint, std::string port, fi_info *userHints)
    : Node(provider, endpoint, userHints)
{
    ctx.opts.src_port = strdup(port.c_str());
}