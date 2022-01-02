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
    std::map<int, int> iterMap;
    if (FLAGS_run_all)
    {
        iterMap = common::getPayloadIterMap();
    }
    else
    {
        iterMap.insert(std::make_pair(FLAGS_payload, FLAGS_iterations));
    }
	for (auto const &iter : iterMap)
	{
		FLAGS_payload = iter.first;
		FLAGS_iterations = iter.second;
		client.sync();

		client.initTxBuffer(FLAGS_payload);

		// Sync after RMA ops are complete
		client.sync();
	}
}
