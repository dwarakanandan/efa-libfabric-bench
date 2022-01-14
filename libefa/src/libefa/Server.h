#include "Node.h"

namespace libefa
{
    class Server : public Node
    {
    public:
        Server(std::string provider, std::string endpoint, std::string port, std::string oobPort, fi_info *userHints);
    };
}