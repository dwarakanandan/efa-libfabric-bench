#include "SendRecvClient.h"

using namespace std;
using namespace libefa;

void SendRecvClient::_pingPongWorker(size_t workerId)
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
						   std::to_string(FLAGS_oob_port + workerId), hints, FLAGS_dst_addr);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	client.startTimer();
	while (true)
	{
		common::workerOperationCounter[workerId] += 2;
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

	client.showTransferStatistics(common::workerOperationCounter[workerId], 2);
}

void SendRecvClient::pingPong()
{
	for (size_t i = 0; i < FLAGS_threads; i++)
	{
		common::workerConnectionStatus.push_back(false);
		common::workerOperationCounter.push_back(0);
		common::workers.push_back(std::thread(&SendRecvClient::_pingPongWorker, this, i));
	}

	for (std::thread &worker : common::workers)
	{
		worker.join();
	}
}

void SendRecvClient::pingPongInject()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	size_t workerId = 0;
	common::workerConnectionStatus.push_back(false);
	common::workerOperationCounter.push_back(0);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
						   std::to_string(FLAGS_oob_port + workerId), hints, FLAGS_dst_addr);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	client.startTimer();
	while (true)
	{
		common::workerOperationCounter[workerId] += 2;
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

	client.showTransferStatistics(common::workerOperationCounter[workerId] / 2, 2);
}

void SendRecvClient::_batchWorker(size_t workerId)
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint,
						   std::to_string(FLAGS_port + workerId),
						   std::to_string(FLAGS_oob_port + workerId),
						   hints, FLAGS_dst_addr);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	client.startTimer();
	while (true)
	{
		common::workerOperationCounter[workerId]++;
		ret = client.rx();
		if (ret)
			return;
		if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
			break;
	}
	client.stopTimer();

	client.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}

void SendRecvClient::batch()
{
	for (size_t i = 0; i < FLAGS_threads; i++)
	{
		common::workerConnectionStatus.push_back(false);
		common::workerOperationCounter.push_back(0);
		common::workers.push_back(std::thread(&SendRecvClient::_batchWorker, this, i));
	}

	for (std::thread &worker : common::workers)
	{
		worker.join();
	}
}

void SendRecvClient::multiRecvBatch()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);
	hints->caps = FI_MSG | FI_MULTI_RECV;
	hints->rx_attr->op_flags = FI_MULTI_RECV;

	size_t workerId = 0;
	common::workerConnectionStatus.push_back(false);
	common::workerOperationCounter.push_back(0);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
						   std::to_string(FLAGS_oob_port + workerId), hints, FLAGS_dst_addr);
	client.ctx.opts.transfer_size = FLAGS_payload;
	client.ctx.cq_attr.format = FI_CQ_FORMAT_DATA;
	client.ctx.opts.options |= FT_OPT_SIZE | FT_OPT_SKIP_MSG_ALLOC | FT_OPT_OOB_SYNC |
							   FT_OPT_OOB_ADDR_EXCH;
	client.init();
	client.initMultiRecv();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	client.startTimer();

	while (true)
	{
		client.postMultiRecv(FLAGS_batch * 10);

		if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
			break;
	}
	client.stopTimer();

	client.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}

void SendRecvClient::latency()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	size_t workerId = 0;
	common::workerConnectionStatus.push_back(false);
	common::workerOperationCounter.push_back(0);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
						   std::to_string(FLAGS_oob_port + workerId), hints, FLAGS_dst_addr);
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

void SendRecvClient::capabilityTest()
{
}

void SendRecvClient::batchLargeBuffer()
{
	int ret;
	fi_info *hints = fi_allocinfo();
	common::setBaseFabricHints(hints);

	size_t workerId = 0;
	common::workerConnectionStatus.push_back(false);
	common::workerOperationCounter.push_back(0);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
						   std::to_string(FLAGS_oob_port + workerId), hints, FLAGS_dst_addr);
	client.enableLargeBufferInit(common::LARGE_BUFFER_SIZE_GBS);
	client.init();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	uint64_t offsets[common::NUM_OFFSET_ADDRS];
	common::generateRandomOffsets(offsets);
	char *addresses[common::NUM_OFFSET_ADDRS];

	for (size_t i = 0; i < common::NUM_OFFSET_ADDRS; i++)
	{
		addresses[i] = (char *)client.ctx.rx_buf + offsets[i];
	}

	int addressIndex = 0;

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	client.startTimer();
	while (true)
	{
		common::workerOperationCounter[workerId]++;
		if (addressIndex == common::NUM_OFFSET_ADDRS - 1)
			addressIndex = 0;

		ret = client.getRxCompletion();
		if (ret)
			return;
		ret = client.postRxBuffer(addresses[addressIndex]);
		if (ret)
			return;
		if (std::chrono::steady_clock::now() - start > std::chrono::seconds(FLAGS_runtime))
			break;
	}
	client.stopTimer();

	client.showTransferStatistics(common::workerOperationCounter[workerId], 1);
}
