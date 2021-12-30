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
		else if (is_benchmark(FLAGS_benchmark_type, T::CAPS))
			startCapsTestServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::RMA))
			startRmaServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::PING_PONG))
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
		else if (is_benchmark(FLAGS_benchmark_type, T::CAPS))
			startCapsTestClient();
		else if (is_benchmark(FLAGS_benchmark_type, T::RMA))
			startRmaClient();
		else if (is_benchmark(FLAGS_benchmark_type, T::PING_PONG))
			startPingPongClient();
	}
}

int main(int argc, char *argv[])
{
	initGflagsFromArgs(argc, argv);

	if (FLAGS_fabinfo)
	{
		fi_info *hints = fi_allocinfo();
		common::setBaseFabricHints(hints);
		Server server = Server(FLAGS_provider, FLAGS_endpoint, hints);
		server.printFabricInfo();
		return 0;
	}

	if (FLAGS_run_all)
	{
		std::map<int, int> payloadIterationMap;
		payloadIterationMap.insert(std::make_pair(1, 5000000));
		payloadIterationMap.insert(std::make_pair(8, 5000000));
		payloadIterationMap.insert(std::make_pair(64, 5000000));
		payloadIterationMap.insert(std::make_pair(512, 1000000));
		payloadIterationMap.insert(std::make_pair(1024, 1000000));
		payloadIterationMap.insert(std::make_pair(4096, 1000000));
		payloadIterationMap.insert(std::make_pair(8192, 1000000));

		for (auto const &x : payloadIterationMap)
		{
			FLAGS_payload = x.first;
			FLAGS_iterations = x.second;
			startNode();
		}
		return 0;
	}
	else
	{
		startNode();
	}

	return 0;
}
