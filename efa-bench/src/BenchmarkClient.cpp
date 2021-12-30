#include "BenchmarkClient.h"

using namespace std;
using namespace libefa;

void startPingPongClient()
{
	int ret;
	fi_info *hints = fi_allocinfo();

	if (!hints)
		return;

	hints->caps = FI_TAGGED;
	hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
	hints->domain_attr->threading = FI_THREAD_DOMAIN;
	hints->tx_attr->tclass = FI_TC_LOW_LATENCY;

	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_dst_addr, hints);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	client.startTimer();
	for (int i = 0; i < FLAGS_iterations; i++)
	{
		ret = client.tx();
		if (ret)
			return;
		ret = client.rx();
		if (ret)
			return;
	}
	client.stopTimer();

	client.showTransferStatistics(FLAGS_iterations, 2);
}

void startPingPongInjectClient()
{
}

void defaultClient()
{
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
}

void startRmaClient()
{
	int ret;

	hints = fi_allocinfo();
	if (!hints)
		return;

	hints->caps = FI_MSG | FI_RMA;
	hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
	hints->domain_attr->threading = FI_THREAD_DOMAIN;
	hints->tx_attr->tclass = FI_TC_BULK_DATA;

	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_dst_addr, hints);
	client.initRmaOp("write");

	client.init();
	client.exchangeKeys();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

    client.startTimer();
    for (int i = 0; i < FLAGS_iterations; i++)
    {
        ret = client.rma();
        if (ret)
            return;
    }
    client.stopTimer();

	// Pingpong once after RMA ops are complete
	client.tx();
	client.rx();

    client.showTransferStatistics(FLAGS_iterations, 1);
}
