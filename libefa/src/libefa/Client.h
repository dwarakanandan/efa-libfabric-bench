#include "Node.h"

namespace libefa
{
    class Client : public Node
    {
    public:
        Client(std::string provider, std::string endpoint, fi_info *userHints, std::string destinationAddress);
    };
}