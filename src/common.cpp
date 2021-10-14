#include "common.h"

using namespace std;

void DEBUG(std::string str)
{
    if (FLAGS_debug)
    {
        std::cout << str << std::endl;
    }
}