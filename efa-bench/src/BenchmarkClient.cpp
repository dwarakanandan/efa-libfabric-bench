#include "BenchmarkClient.h"

using namespace std;
using namespace libefa;

void pingPongClient(std::string port)
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, port, hints, FLAGS_dst_addr);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	client.startTimer();
	while (true)
	{
		common::operationCounter += 2;
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

	client.showTransferStatistics(common::operationCounter / 2, 2);
}

void startPingPongClient()
{
    std::thread worker0(pingPongClient, "10000");
    std::thread worker1(pingPongClient, "10001");
    std::thread worker2(pingPongClient, "10002");
    std::thread worker3(pingPongClient, "10003");
    worker0.join();
    worker1.join();
    worker2.join();
    worker3.join();
}

void startPingPongInjectClient()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, "10000", hints, FLAGS_dst_addr);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	client.startTimer();
	while (true)
	{
		common::operationCounter += 2;
		ret = client.inject();
		if (ret)
			return;
		ret = client.rx();
		if (ret)
			return;
		if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
			break;
	}
	client.stopTimer();

	client.showTransferStatistics(common::operationCounter / 2, 2);
}

void defaultClient()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, "10000", hints, FLAGS_dst_addr);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	client.startTimer();
	while (true)
	{
		common::operationCounter++;
		ret = client.rx();
		if (ret)
			return;
		if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
			break;
	}
	client.stopTimer();

	client.showTransferStatistics(common::operationCounter, 1);
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

	Client client = Client(FLAGS_provider, FLAGS_endpoint, "10000", hints, FLAGS_dst_addr);
	client.initRmaOp(FLAGS_rma_op);

	client.init();
	client.exchangeKeys();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	// Sync after RMA ops are complete
	client.sync();
}
