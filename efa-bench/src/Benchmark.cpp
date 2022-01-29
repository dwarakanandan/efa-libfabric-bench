#include "Common.h"
#include "BenchmarkNode.h"

int main(int argc, char *argv[])
{
	initGflagsFromArgs(argc, argv);

	if (FLAGS_fabinfo)
	{
		BenchmarkNode().getFabInfo();
		return 0;
	}

	BenchmarkNode().run();

	return 0;
}
