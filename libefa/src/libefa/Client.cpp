#include "Client.h"

libefa::Client::Client(std::string provider, std::string endpoint, fi_info *userHints, std::string destinationAddress)
    : Node(provider, endpoint, userHints)
{
    opts.dst_addr = strdup(destinationAddress.c_str());
}