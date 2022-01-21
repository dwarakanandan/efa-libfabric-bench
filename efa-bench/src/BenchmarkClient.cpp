#include "BenchmarkClient.h"

using namespace std;
using namespace libefa;

void pingPongClient(std::string port, std::string oobPort)
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, port, oobPort, hints, FLAGS_dst_addr);
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
	std::thread worker0(pingPongClient, "10000", "9000");
	std::thread worker1(pingPongClient, "10001", "9001");
	std::thread worker2(pingPongClient, "10002", "9002");
	std::thread worker3(pingPongClient, "10003", "9003");
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

	Client client = Client(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints, FLAGS_dst_addr);
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

	Client client = Client(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints, FLAGS_dst_addr);
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

void startBatchClient()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints, FLAGS_dst_addr);
	client.ctx.opts.comp_method = FT_COMP_SREAD;
	client.ctx.opts.options |= FT_OPT_ALLOC_MULT_MR;
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	client.startTimer();

	void* rx_buffers[FLAGS_batch];
	for (size_t i = 0; i < FLAGS_batch; i++)
	{
		rx_buffers[i] = malloc(10000);
	}
	
	while (true)
	{
		for (size_t i = 0; i < FLAGS_batch; i++)
		{
			client.postRx(rx_buffers[i]);
		}
		client.getNRxCompletion(FLAGS_batch);

		if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
			break;
	}
	client.stopTimer();

	client.showTransferStatistics(common::operationCounter, 1);
}

void startLatencyTestClient()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints, FLAGS_dst_addr);
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

void startCapsTestClient()
{
}

void startRmaClient()
{
	int ret;

	fi_info *hints = fi_allocinfo();
	common::setRmaFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, "10000", "9000", hints, FLAGS_dst_addr);
	client.initRmaOp(FLAGS_rma_op);

	client.init();
	client.exchangeKeys();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	// Sync after RMA ops are complete
	client.sync();
}
