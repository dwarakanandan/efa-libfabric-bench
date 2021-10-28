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
        * This init function has predefined modes and capablities for the EFA fabric adapter
        * @param provider : Name of the fabric provider. Eg: sockets, efa
        * @param endpoint : Endpoint type. Eg: dgram, rdm
        */
        int initFabricInfo(std::string provider, std::string endpoint);

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