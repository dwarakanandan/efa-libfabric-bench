#include "Node.h"
#include "FabricUtil.h"

namespace libefa
{
    class Client : public Node
    {
        std::string destinationAddress;

        virtual int initFabric(fi_info* hints);

        virtual int ctrlInit();

        virtual int ctrlSync();

        int fabricGetaddrinfo(struct addrinfo **results);

    public:
        Client(std::string provider, std::string endpoint, bool isTagged, std::string destinationAddress, uint16_t port);

        virtual ConnectionContext getConnectionContext();

        virtual int init(fi_info* hints);
    };
}