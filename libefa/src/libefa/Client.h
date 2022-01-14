#include "Node.h"

namespace libefa
{
    class Client : public Node
    {
    public:
        Client(std::string provider, std::string endpoint, std::string port, fi_info *userHints, std::string destinationAddress);
    };
}