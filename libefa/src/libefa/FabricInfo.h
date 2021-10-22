#include "includes.h"
#include "Util.h"

namespace libefa
{
    class FabricInfo
    {
        struct fi_info *hints, *info;

    public:
        void loadFabricInfo(std::string providerName);

        void printFabricInfoShort();

        void printFabricInfoLong();
    };
}