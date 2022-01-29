#include "RmaClient.h"

using namespace std;
using namespace libefa;

void RmaClient::_rmaWorker(size_t workerId)
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

void RmaClient::_spawnRmaWorkers(size_t numWorkers)
{
	for (size_t i = 0; i < numWorkers; i++)
	{
		common::workers.push_back(std::thread(&RmaClient::_rmaWorker, this, i));
	}

	for (std::thread &worker : common::workers)
	{
		worker.join();
	}
}

void RmaClient::rma()
{
	this->_spawnRmaWorkers(FLAGS_threads);
}

void RmaClient::batch()
{
	this->_spawnRmaWorkers(FLAGS_threads);
}

void RmaClient::inject()
{
	this->_spawnRmaWorkers(1);
}

void RmaClient::batchSelectiveCompletion()
{
	this->_spawnRmaWorkers(1);
}

void RmaClient::batchLargeBuffer()
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