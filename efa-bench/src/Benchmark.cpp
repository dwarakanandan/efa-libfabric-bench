#include "Common.h"
#include "BenchmarkClient.h"
#include "BenchmarkServer.h"

using namespace std;
using namespace libefa;
using namespace common;

void startNode()
{
	if (is_node(FLAGS_mode, T::SERVER))
	{
		if (is_benchmark(FLAGS_benchmark_type, T::LATENCY))
			startLatencyTestServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::INJECT))
			startPingPongInjectServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::BATCH))
			startTaggedBatchServer();
		else
			startPingPongServer();
	}

	if (is_node(FLAGS_mode, T::CLIENT))
	{
		if (is_benchmark(FLAGS_benchmark_type, T::LATENCY))
			startLatencyTestClient();
		else if (is_benchmark(FLAGS_benchmark_type, T::INJECT))
			startPingPongInjectClient();
		else if (is_benchmark(FLAGS_benchmark_type, T::BATCH))
			startTaggedBatchClient();
		else
			startPingPongClient();
	}
}

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
		payloadIterationMap.insert(std::make_pair(1, 5000000));
		payloadIterationMap.insert(std::make_pair(8, 5000000));
		payloadIterationMap.insert(std::make_pair(64, 5000000));
		payloadIterationMap.insert(std::make_pair(512, 5000000));
		payloadIterationMap.insert(std::make_pair(1024, 5000000));
		payloadIterationMap.insert(std::make_pair(4096, 5000000));
		payloadIterationMap.insert(std::make_pair(8192, 5000000));

		for (auto const &x : payloadIterationMap)
		{
			FLAGS_payload = x.first;
			FLAGS_iterations = x.second;
			startNode();
		}
		return 0;
	}

	startNode();

	return 0;
}
