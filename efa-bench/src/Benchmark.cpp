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
			startBatchServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::CAPS))
			startCapsTestServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::RMA))
			startRmaServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::PING_PONG))
			startPingPongServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::RMA_BATCH))
			startRmaBatchServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::RMA_INJECT))
			startRmaInjectServer();
		else if (is_benchmark(FLAGS_benchmark_type, T::RMA_SEL_COMP))
			startRmaSelectiveCompletionServer();
	}

	if (is_node(FLAGS_mode, T::CLIENT))
	{
		if (is_benchmark(FLAGS_benchmark_type, T::LATENCY))
			startLatencyTestClient();
		else if (is_benchmark(FLAGS_benchmark_type, T::INJECT))
			defaultClient();
		else if (is_benchmark(FLAGS_benchmark_type, T::BATCH))
			startBatchClient();
		else if (is_benchmark(FLAGS_benchmark_type, T::CAPS))
			startCapsTestClient();
		else if (is_benchmark(FLAGS_benchmark_type, T::RMA) || is_benchmark(FLAGS_benchmark_type, T::RMA_BATCH) || is_benchmark(FLAGS_benchmark_type, T::RMA_INJECT) || is_benchmark(FLAGS_benchmark_type, T::RMA_SEL_COMP))
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
		Server server = Server(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints);
		server.printFabricInfo();
		return 0;
	}

	startNode();

	return 0;
}
