#include "Server.h"

libefa::Server::Server(std::string provider, std::string endpoint, fi_info *userHints)
    : Node(provider, endpoint, userHints)
{
}