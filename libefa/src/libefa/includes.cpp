#include "includes.h"

bool libefa::ENABLE_DEBUG = false;
bool libefa::ENABLE_RXTX_VERIFICATION = false;

void DEBUG(std::string str)
{
	if (libefa::ENABLE_DEBUG)
	{
		std::cout << str << std::endl;
	}
}