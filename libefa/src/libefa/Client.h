#include "includes.h"
#include "FabricUtil.h"
#include "ConnectionContext.h"

namespace libefa
{
    class Client
    {
        ConnectionContext ctx;
        std::string provider;
        std::string destinationAddress;
        uint16_t destinationPort;

        int initFabric();

        int ctrlInit();

        int fabricGetaddrinfo(struct addrinfo **results);

        int ctrlSync();

    public:
        Client(std::string provider, std::string destinationAddress, uint16_t destinationPort);

        ConnectionContext getConnectionContext();

        int startDgramClient();
    };
}