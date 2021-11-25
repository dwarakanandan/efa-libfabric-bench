#include "BenchmarkClient.h"

using namespace std;
using namespace libefa;

void startPingPongClient()
{
	int ret;
	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_dst_addr, FLAGS_dst_port);
	ret = client.init();
	if (ret)
		return;

	ConnectionContext clientCtx = client.getConnectionContext();

	DEBUG("CLIENT: Starting data transfer\n\n");
	clientCtx.startTimekeeper();

	for (int i = 0; i < FLAGS_iterations; i++)
	{
		ret = FabricUtil::tx(&clientCtx, clientCtx.ep, FLAGS_payload);
		if (ret)
			return;
		ret = FabricUtil::rx(&clientCtx, clientCtx.ep, FLAGS_payload);
		if (ret)
			return;
	}

	clientCtx.stopTimekeeper();
	DEBUG("CLIENT: Completed data transfer\n\n");

	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  clientCtx.cnt_ack_msg, clientCtx.start, clientCtx.end, 2);
}

void startPingPongInjectClient()
{
	int ret;
	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_dst_addr, FLAGS_dst_port);
	ret = client.init();
	if (ret)
		return;

	ConnectionContext clientCtx = client.getConnectionContext();

	DEBUG("CLIENT: Starting data transfer\n\n");
	clientCtx.startTimekeeper();

	for (int i = 0; i < FLAGS_iterations; i++)
	{
		ret = FabricUtil::inject(&clientCtx, clientCtx.ep, FLAGS_payload);
		if (ret)
			return;
		ret = FabricUtil::rx(&clientCtx, clientCtx.ep, FLAGS_payload);
		if (ret)
			return;
	}

	clientCtx.stopTimekeeper();
	DEBUG("CLIENT: Completed data transfer\n\n");

	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  clientCtx.cnt_ack_msg, clientCtx.start, clientCtx.end, 2);
}

void defaultClient()
{
	int ret;
	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_dst_addr, FLAGS_dst_port);
	ret = client.init();
	if (ret)
		return;

	ConnectionContext clientCtx = client.getConnectionContext();

	clientCtx.timeout_sec = -1;

	DEBUG("CLIENT: Receiving data transfer\n\n");
	for (int i = 1; i <= FLAGS_iterations; i++)
	{
		FabricUtil::rx(&clientCtx, clientCtx.ep, FLAGS_payload);
	}
	DEBUG("CLIENT: Completed Receiving data transfer\n\n");
}

void startTaggedBatchClient()
{
	defaultClient();
}

void startLatencyTestClient()
{
	defaultClient();
}