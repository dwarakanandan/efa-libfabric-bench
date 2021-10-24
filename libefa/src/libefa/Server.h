#include "includes.h"
#include "FabricUtil.h"
#include "ConnectionContext.h"

namespace libefa
{
    class Server
    {
        ConnectionContext ctx;
        std::string provider;
        uint16_t sourcePort;

        int initFabric();

        int ctrlInit();

        int ctrlSync();

    public:
        Server(std::string provider, uint16_t port);

        ConnectionContext getConnectionContext();

        int startDgramServer();
    };
}