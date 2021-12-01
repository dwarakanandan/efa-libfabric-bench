#include "BenchmarkServer.h"

using namespace std;
using namespace libefa;

void startPingPongServer()
{
	int ret;
	Server server = Server(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_src_port);
	ret = server.init(NULL);
	if (ret)
		return;

	ConnectionContext serverCtx = server.getConnectionContext();

	DEBUG("SERVER: Starting data transfer\n\n");
	serverCtx.startTimekeeper();

	for (int i = 0; i < FLAGS_iterations; i++)
	{
		ret = FabricUtil::rx(&serverCtx, serverCtx.ep, FLAGS_payload);
		if (ret)
			return;
		ret = FabricUtil::tx(&serverCtx, serverCtx.ep, FLAGS_payload);
		if (ret)
			return;
	}

	serverCtx.stopTimekeeper();
	DEBUG("SERVER: Completed data transfer\n\n");

	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  serverCtx.cnt_ack_msg, serverCtx.start, serverCtx.end, 2);
}

void startPingPongInjectServer()
{
	int ret;
	Server server = Server(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_src_port);
	ret = server.init(NULL);
	if (ret)
		return;

	ConnectionContext serverCtx = server.getConnectionContext();

	DEBUG("SERVER: Starting data transfer\n\n");
	serverCtx.startTimekeeper();

	for (int i = 0; i < FLAGS_iterations; i++)
	{
		ret = FabricUtil::rx(&serverCtx, serverCtx.ep, FLAGS_payload);
		if (ret)
			return;
		ret = FabricUtil::inject(&serverCtx, serverCtx.ep, FLAGS_payload);
		if (ret)
			return;
	}

	serverCtx.stopTimekeeper();
	DEBUG("SERVER: Completed data transfer\n\n");

	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  serverCtx.cnt_ack_msg, serverCtx.start, serverCtx.end, 2);
}

void startTaggedBatchServer()
{
	int ret;
	Server server = Server(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_src_port);
	ret = server.init(NULL);
	if (ret)
		return;

	ConnectionContext serverCtx = server.getConnectionContext();

	// std::cout << "Press any key to transmit..." << std::endl;
	// std::cin.ignore();

	DEBUG("SERVER: Starting data transfer\n\n");

	FabricUtil::fillBuffer((char *)serverCtx.tx_buf + serverCtx.tx_prefix_size, FLAGS_payload);

	serverCtx.startTimekeeper();
	int numTxRetries = 0;
	int numCqObtained = 0;
	int cqTry = FLAGS_batch * FLAGS_cq_try;

	for (int i = 1; i <= FLAGS_iterations; i++)
	{
		ret = fi_tsend(serverCtx.ep, serverCtx.tx_buf, FLAGS_payload + serverCtx.tx_prefix_size,
					   fi_mr_desc(serverCtx.mr), serverCtx.remote_fi_addr, TAG, NULL);
		while (ret == -FI_EAGAIN)
		{
			if (numCqObtained < i)
			{
				ret = FabricUtil::getCqCompletion(serverCtx.txcq, &(serverCtx.tx_cq_cntr), serverCtx.tx_cq_cntr + 1, -1);
				if (ret)
				{
					printf("SERVER: getCqCompletion failed\n\n");
					exit(1);
				}
				numCqObtained++;
			}

			// printf("fi_tsend retry iteration %d\n", i);
			ret = fi_tsend(serverCtx.ep, serverCtx.tx_buf, FLAGS_payload + serverCtx.tx_prefix_size,
						   fi_mr_desc(serverCtx.mr), serverCtx.remote_fi_addr, TAG, NULL);
			numTxRetries++;
		}
		if ((i - numCqObtained) > FLAGS_batch)
		{
			ret = FabricUtil::getCqCompletion(serverCtx.txcq, &(serverCtx.tx_cq_cntr), serverCtx.tx_cq_cntr + cqTry, -1);
			if (ret)
			{
				printf("SERVER: getCqCompletion failed\n\n");
				exit(1);
			}
			numCqObtained += cqTry;
		}
	}

	DEBUG("Num TX Retries %d\n\n", numTxRetries);
	DEBUG("CQ Already obtained %d\n\n", numCqObtained);
	ret = FabricUtil::getCqCompletion(serverCtx.txcq, &(serverCtx.tx_cq_cntr),
									  serverCtx.tx_cq_cntr + (FLAGS_iterations - numCqObtained), -1);
	if (ret)
	{
		printf("SERVER: getCqCompletion failed\n\n");
		exit(1);
	}

	serverCtx.stopTimekeeper();
	DEBUG("SERVER: Completed data transfer\n\n");
	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  serverCtx.tx_cq_cntr, serverCtx.start, serverCtx.end, 1);
}

void startLatencyTestServer()
{
	int ret;
	Server server = Server(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_src_port);
	ret = server.init(NULL);
	if (ret)
		return;

	ConnectionContext serverCtx = server.getConnectionContext();

	DEBUG("SERVER: Starting data transfer\n\n");

	FabricUtil::fillBuffer((char *)serverCtx.tx_buf + serverCtx.tx_prefix_size, FLAGS_payload);

	serverCtx.startTimekeeper();
	for (int i = 1; i <= FLAGS_iterations; i++)
	{
		fi_tsend(serverCtx.ep, serverCtx.tx_buf, FLAGS_payload + serverCtx.tx_prefix_size,
				 fi_mr_desc(serverCtx.mr), serverCtx.remote_fi_addr, TAG, NULL);
		FabricUtil::getCqCompletion(serverCtx.txcq, &(serverCtx.tx_cq_cntr), serverCtx.tx_cq_cntr + 1, -1);
	}
	serverCtx.stopTimekeeper();
	DEBUG("SERVER: Completed data transfer\n\n");

	printf("SERVER: CQ Count: %lu\n\n", serverCtx.tx_cq_cntr);

	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  serverCtx.tx_cq_cntr, serverCtx.start, serverCtx.end, 1);
}

void startCapsTestServer()
{
	int ret;
	Server server = Server(FLAGS_provider, FLAGS_endpoint, FLAGS_tagged, FLAGS_src_port);
	fi_info* hints = fi_allocinfo();

	hints->fabric_attr->prov_name = const_cast<char *>(FLAGS_provider.c_str());
	hints->ep_attr->type = FI_EP_RDM;
	hints->mode = FI_MSG_PREFIX;
	hints->caps = FI_TAGGED | FI_SEND;
	hints->domain_attr->mode = ~0;
    hints->domain_attr->mr_mode = FI_MR_LOCAL | FI_MR_VIRT_ADDR | FI_MR_ALLOCATED | FI_MR_PROV_KEY;

	ret = server.init(hints);
	if (ret)
		return;

	ConnectionContext serverCtx = server.getConnectionContext();

	DEBUG("SERVER: Starting data transfer\n\n");
	serverCtx.startTimekeeper();

	for (int i = 0; i < FLAGS_iterations; i++)
	{
		ret = FabricUtil::rx(&serverCtx, serverCtx.ep, FLAGS_payload);
		if (ret)
			return;
		ret = FabricUtil::tx(&serverCtx, serverCtx.ep, FLAGS_payload);
		if (ret)
			return;
	}

	serverCtx.stopTimekeeper();
	DEBUG("SERVER: Completed data transfer\n\n");

	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  serverCtx.cnt_ack_msg, serverCtx.start, serverCtx.end, 2);
}