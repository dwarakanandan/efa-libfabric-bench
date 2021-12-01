#include "Node.h"
#include "FabricUtil.h"

namespace libefa
{
    class Server : public Node
    {
        virtual int initFabric(fi_info* hints);

        virtual int ctrlInit();

        virtual int ctrlSync();

    public:
        Server(std::string provider, std::string endpoint, bool isTagged, uint16_t port);

        virtual ConnectionContext getConnectionContext();

        virtual int init(fi_info* hints);

        virtual int exchangeRmaIov();
    };
}