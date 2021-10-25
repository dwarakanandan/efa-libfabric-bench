#include "Node.h"
#include "FabricUtil.h"

namespace libefa
{
    class Server : public Node
    {
        virtual int initFabric();

        virtual int ctrlInit();

        virtual int ctrlSync();

    public:
        Server(std::string provider, std::string endpoint, uint16_t port);

        virtual ConnectionContext getConnectionContext();

        virtual int init();
    };
}