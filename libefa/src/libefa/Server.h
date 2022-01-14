#include "Node.h"

namespace libefa
{
    class Server : public Node
    {
    public:
        Server(std::string provider, std::string endpoint, std::string port, fi_info *userHints);
    };
}