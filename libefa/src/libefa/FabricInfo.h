#include "includes.h"
#include "Util.h"

namespace libefa
{
    class FabricInfo
    {
        struct fi_info *hints, *info;

    public:
        /** 
        * @param provider : Name of the fabric provider. Eg: sockets, efa
        */
        FabricInfo(std::string provider);

        void printFabricInfoShort();

        void printFabricInfoLong();
    };
}