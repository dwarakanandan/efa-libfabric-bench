#include "Node.h"

namespace libefa
{
    class Client : public Node
    {
    public:
        Client(std::string provider, std::string endpoint, std::string destinationAddress, fi_info *userHints);
    };
}