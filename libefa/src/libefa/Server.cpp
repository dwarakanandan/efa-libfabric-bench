#include "Server.h"

libefa::Server::Server(std::string provider, std::string endpoint, std::string port, std::string oobPort, fi_info *userHints)
    : Node(provider, endpoint, oobPort, userHints)
{
    ctx.opts.src_port = strdup(port.c_str());
}