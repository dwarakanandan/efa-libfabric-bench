#include "BenchmarkClient.h"

using namespace std;
using namespace libefa;

void startPingPongClient()
{
	int ret;
	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_dst_addr, FLAGS_dst_port);
	ret = client.init(NULL);
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
	ret = client.init(NULL);
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
	ret = client.init(NULL);
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

void startCapsTestClient()
{
	int ret;
	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_dst_addr, FLAGS_dst_port);
	fi_info *hints = fi_allocinfo();

	hints->fabric_attr->prov_name = const_cast<char *>(FLAGS_provider.c_str());
	hints->ep_attr->type = FI_EP_RDM;
	hints->mode = FI_MSG_PREFIX;
	hints->caps = FI_TAGGED | FI_RECV;
	hints->domain_attr->mode = ~0;
	hints->domain_attr->mr_mode = FI_MR_LOCAL | FI_MR_VIRT_ADDR | FI_MR_ALLOCATED | FI_MR_PROV_KEY;

	ret = client.init(hints);
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

void printMemRegion(ConnectionContext *ctx)
{
	printf("\nRX_BUFFER: ");
	for (int i = 0; i < 5; i++)
	{
		printf("[%c] ", *((char*)ctx->rx_buf + i));
	}
	printf("\n\n");
}

void startRmaClient()
{
	int ret;
	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_dst_addr, FLAGS_dst_port);
	fi_info *hints = fi_allocinfo();

	hints->fabric_attr->prov_name = const_cast<char *>(FLAGS_provider.c_str());
	hints->ep_attr->type = FI_EP_RDM;
	hints->mode = FI_MSG_PREFIX;
	hints->caps = FI_MSG | FI_RMA | FI_REMOTE_READ | FI_REMOTE_WRITE;
	hints->domain_attr->mode = ~0;
	hints->domain_attr->mr_mode = FI_MR_LOCAL | FI_MR_VIRT_ADDR | FI_MR_ALLOCATED | FI_MR_PROV_KEY;

	ret = client.init(hints);
	if (ret)
		return;

	ret = client.exchangeRmaIov();
	if (ret)
		return;

	ConnectionContext clientCtx = client.getConnectionContext();

	printMemRegion(&clientCtx);

	DEBUG("CLIENT: Waiting for RMA request\n\n");

	sleep(3);
	printMemRegion(&clientCtx);

	DEBUG("CLIENT: Received RMA request\n\n");
}
