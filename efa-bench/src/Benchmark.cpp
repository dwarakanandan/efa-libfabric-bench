#include "BenchmarkClient.h"
#include "BenchmarkServer.h"

using namespace std;
using namespace libefa;

int main(int argc, char *argv[])
{
	initGflagsFromArgs(argc, argv);

	if (FLAGS_debug)
	{
		libefa::ENABLE_DEBUG = true;
	}

	if (FLAGS_fabinfo)
	{
		FabricInfo fabInfo = FabricInfo();
		fabInfo.initFabricInfo(FLAGS_provider, FLAGS_endpoint);
		fabInfo.printFabricInfoLong();
		return 0;
	}

	if (FLAGS_run_all)
	{
		std::map<int, int> payloadIterationMap;
		payloadIterationMap.insert(std::make_pair(1, 1000000));
		payloadIterationMap.insert(std::make_pair(8, 1000000));
		payloadIterationMap.insert(std::make_pair(64, 1000000));
		payloadIterationMap.insert(std::make_pair(512, 1000000));
		payloadIterationMap.insert(std::make_pair(1024, 1000000));
		payloadIterationMap.insert(std::make_pair(4096, 1000000));
		payloadIterationMap.insert(std::make_pair(8192, 500000));

		for (auto const &x : payloadIterationMap)
		{
			FLAGS_payload = x.first;
			FLAGS_iterations = x.second;
			if (FLAGS_mode == "server")
			{
				startTaggedBatchServer();
			}

			if (FLAGS_mode == "client")
			{
				startTaggedBatchClient();
			}
		}
		return 0;
	}

	if (FLAGS_mode == "server")
	{
		startPingPongInjectServer();
	}

	if (FLAGS_mode == "client")
	{
		startPingPongInjectClient();
	}

	return 0;
}
