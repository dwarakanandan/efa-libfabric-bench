#include "BenchmarkClient.h"

using namespace std;
using namespace libefa;

void startPingPongClient()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, hints, FLAGS_dst_addr);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    client.startTimer();
    while (true)
    {
        common::iterationCounter++;
        ret = client.tx();
        if (ret)
            return;
        ret = client.rx();
        if (ret)
            return;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
            break;
    }
    client.stopTimer();

    client.showTransferStatistics(common::iterationCounter, 2);
}

void startPingPongInjectClient()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, hints, FLAGS_dst_addr);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	client.startTimer();
	for (int i = 0; i < FLAGS_iterations; i++)
	{
		ret = client.inject();
		if (ret)
			return;
		ret = client.rx();
		if (ret)
			return;
	}
	client.stopTimer();

	client.showTransferStatistics(FLAGS_iterations, 2);
}

void defaultClient()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, hints, FLAGS_dst_addr);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	client.startTimer();
	for (int i = 0; i < FLAGS_iterations; i++)
	{
		ret = client.rx();
		if (ret)
			return;
	}
	client.stopTimer();

	client.showTransferStatistics(FLAGS_iterations, 1);
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

	fi_info *hints = fi_allocinfo();
	common::setRmaFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, hints, FLAGS_dst_addr);
	client.initRmaOp(FLAGS_rma_op);

	client.init();
	client.exchangeKeys();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	// Sync after RMA ops are complete
	client.sync();
}
