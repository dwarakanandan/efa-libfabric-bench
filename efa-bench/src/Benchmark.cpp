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
		fabInfo.initFabricInfo(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged);
		fabInfo.printFabricInfoLong();
		return 0;
	}

	if (FLAGS_run_all)
	{
		std::map<int, int> payloadIterationMap;
		payloadIterationMap.insert(std::make_pair(1, 200000));
		payloadIterationMap.insert(std::make_pair(8, 200000));
		payloadIterationMap.insert(std::make_pair(64, 200000));
		payloadIterationMap.insert(std::make_pair(512, 200000));
		payloadIterationMap.insert(std::make_pair(1024, 100000));
		payloadIterationMap.insert(std::make_pair(4096, 100000));
		payloadIterationMap.insert(std::make_pair(8192, 50000));

		for (auto const &x : payloadIterationMap)
		{
			FLAGS_payload = x.first;
			FLAGS_iterations = x.second;
			if (FLAGS_mode == "server")
			{
				if (FLAGS_inject)
				{
					startPingPongInjectServer();
				}
				else
				{
					startPingPongServer();
				}
			}

			if (FLAGS_mode == "client")
			{
				if (FLAGS_inject)
				{
					startPingPongInjectClient();
				}
				else
				{
					startPingPongClient();
				}
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
