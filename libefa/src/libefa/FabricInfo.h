#pragma once

#include "includes.h"

namespace libefa
{
    class FabricInfo
    {
    public:
        struct fi_info *info;
        struct fi_info *hints;

        FabricInfo()
        {
            hints = fi_allocinfo();
        }

        /** 
        * @param provider : Name of the fabric provider. Eg: sockets, efa
        */
        int initFabricInfo(std::string provider);

        /** 
        * @param provider : Name of the fabric provider. Eg: sockets, efa
        * @param hints : Hints used to filter fabric and endpoint properties
        */
        int initFabricInfo(std::string provider, struct fi_info *hints);

        void printFabricInfoShort();

        void printFabricInfoLong();

        void printFabricInfoBanner();
    };
}