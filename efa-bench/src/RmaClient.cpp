#include "RmaClient.h"

using namespace std;
using namespace libefa;

void startRmaClient()
{
	int ret;

	size_t workerId = 0;

	fi_info *hints = fi_allocinfo();
	common::setRmaFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
						   std::to_string(FLAGS_oob_port + workerId), hints, FLAGS_dst_addr);
	client.initRmaOp(FLAGS_rma_op);

	client.init();
	client.exchangeKeys();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	// Sync after RMA ops are complete
	client.sync();
}

void _rmaBatchClientWorker(size_t workerId)
{
	int ret;

	fi_info *hints = fi_allocinfo();
	common::setRmaFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
						   std::to_string(FLAGS_oob_port + workerId), hints, FLAGS_dst_addr);
	client.initRmaOp(FLAGS_rma_op);

	client.init();
	client.exchangeKeys();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	// Sync after RMA ops are complete
	client.sync();
}

void startRmaBatchClient()
{
	for (size_t i = 0; i < FLAGS_threads; i++)
	{
		common::workerConnectionStatus.push_back(false);
		common::workerOperationCounter.push_back(0);
		common::workers.push_back(std::thread(_rmaBatchClientWorker, i));
	}

	for (std::thread &worker : common::workers)
	{
		worker.join();
	}
}

void startRmaLargeBufferClient()
{
	int ret;

	size_t workerId = 0;

	fi_info *hints = fi_allocinfo();
	common::setRmaFabricHints(hints);

	Client client = Client(FLAGS_provider, FLAGS_endpoint, std::to_string(FLAGS_port + workerId),
						   std::to_string(FLAGS_oob_port + workerId), hints, FLAGS_dst_addr);
	client.enableLargeBufferInit(common::LARGE_BUFFER_SIZE_GBS);
	client.initRmaOp(FLAGS_rma_op);

	client.init();
	client.exchangeKeys();
	client.sync();

	client.initTxBuffer(FLAGS_payload);

	// Sync after RMA ops are complete
	client.sync();
}