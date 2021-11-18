#include <iostream>
#include <libefa/FabricInfo.h>
#include <libefa/Server.h>
#include <libefa/Client.h>
#include <libefa/PerformancePrinter.h>

#include "cmd_line.h"

using namespace std;
using namespace libefa;

void initCommandLine(int argc, char *argv[])
{
	gflags::SetUsageMessage(gflags_cmdline_message);
	gflags::SetVersionString(gflags_version_string);

	if (argc == 1)
	{
		gflags::ShowUsageWithFlags(argv[0]);
		exit(0);
	}

	gflags::ParseCommandLineFlags(&argc, &argv, true);
}

void startServer()
{
	int ret;
	Server server = Server(FLAGS_provider, FLAGS_endpoint, FLAGS_src_port);
	ret = server.init();
	if (ret)
		return;

	ConnectionContext serverCtx = server.getConnectionContext();

	printf("SERVER: Starting data transfer\n\n");
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
	printf("SERVER: Completed data transfer\n\n");

	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  serverCtx.cnt_ack_msg, serverCtx.start, serverCtx.end, 2);
}

void startClient()
{
	int ret;
	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_dst_addr, FLAGS_dst_port);
	ret = client.init();
	if (ret)
		return;

	ConnectionContext clientCtx = client.getConnectionContext();

	printf("CLIENT: Starting data transfer\n\n");
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
	printf("CLIENT: Completed data transfer\n\n");

	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  clientCtx.cnt_ack_msg, clientCtx.start, clientCtx.end, 2);
}

void startServer2()
{
	int ret;
	Server server = Server(FLAGS_provider, FLAGS_endpoint, FLAGS_src_port);
	ret = server.init();
	if (ret)
		return;

	ConnectionContext serverCtx = server.getConnectionContext();

	std::cout << "Press any key to transmit..." << std::endl;
	std::cin.ignore();

	printf("SERVER: Starting data transfer\n\n");
	serverCtx.startTimekeeper();

	for (int i = 1; i <= FLAGS_iterations; i++)
	{
		//ret = FabricUtil::tx(&serverCtx, serverCtx.ep, FLAGS_payload);
		FabricUtil::fillBuffer((char *)serverCtx.tx_buf + serverCtx.tx_prefix_size, FLAGS_payload);
		ret = FabricUtil::postTx(&serverCtx, serverCtx.ep, FLAGS_payload + serverCtx.tx_prefix_size, serverCtx.tx_ctx_ptr);
		if (ret)
			return;

		if (i % FLAGS_batch == 0)
		{
			ret = FabricUtil::rx(&serverCtx, serverCtx.ep, FLAGS_payload);
			if (ret)
				return;
		}
	}

	serverCtx.stopTimekeeper();
	printf("SERVER: Completed data transfer\n\n");

	std::cout << "TX CQ count: " << serverCtx.tx_seq << std::endl;
	std::cout << "ACK count: " << serverCtx.cnt_ack_msg << std::endl;

	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  serverCtx.cnt_ack_msg, serverCtx.start, serverCtx.end, 1);
}

void startClient2()
{
	int ret;
	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_dst_addr, FLAGS_dst_port);
	ret = client.init();
	if (ret)
		return;

	ConnectionContext clientCtx = client.getConnectionContext();

	clientCtx.timeout_sec = -1;

	printf("CLIENT: Receiving data transfer\n\n");

	for (int i = 1; i <= FLAGS_iterations; i++)
	{
		ret = FabricUtil::rx(&clientCtx, clientCtx.ep, FLAGS_payload);
		if (ret)
			return;

		if (i % FLAGS_batch == 0)
		{
			ret = FabricUtil::tx(&clientCtx, clientCtx.ep, FLAGS_payload);
			if (ret)
				return;
		}
	}

	printf("CLIENT: Completed Receiving data transfer\n\n");
}

int sendBatch(ConnectionContext serverCtx, int batchCount)
{
	int ret = 0;
	for (int i = 1; i <= FLAGS_batch; i++)
	{
		ret = fi_tsend(serverCtx.ep, serverCtx.tx_buf, FLAGS_payload + serverCtx.tx_prefix_size,
					   fi_mr_desc(serverCtx.mr), serverCtx.remote_fi_addr, TAG, NULL);
		while (ret == -FI_EAGAIN)
		{
			printf("fi_tsend retry batch: %d iteration %d\n", batchCount, i);
			ret = fi_tsend(serverCtx.ep, serverCtx.tx_buf, FLAGS_payload + serverCtx.tx_prefix_size,
						   fi_mr_desc(serverCtx.mr), serverCtx.remote_fi_addr, TAG, NULL);
		}
		if (ret)
		{
			printf("SERVER: fi_tsend failed\n\n");
			return ret;
		}
	}
	return ret;
}

void startServer3()
{
	int ret;
	Server server = Server(FLAGS_provider, FLAGS_endpoint, FLAGS_src_port);
	ret = server.init();
	if (ret)
		return;

	ConnectionContext serverCtx = server.getConnectionContext();

	std::cout << "Press any key to transmit..." << std::endl;
	std::cin.ignore();

	printf("SERVER: Starting data transfer\n\n");

	FabricUtil::fillBuffer((char *)serverCtx.tx_buf + serverCtx.tx_prefix_size, FLAGS_payload);

	serverCtx.startTimekeeper();
	for (int i = 1; i <= (FLAGS_iterations / FLAGS_batch); i++)
	{
		// Only during the first batch queue up "batch" number of packets
		if (i == 1)
		{
			sendBatch(serverCtx, i);
		}

		// For every except the last send "batch" number of packets (This includes the first batch too)
		if (i < (FLAGS_iterations / FLAGS_batch))
		{
			sendBatch(serverCtx, i);
		}

		// Wait for "batch" number of completions
		ret = FabricUtil::getCqCompletion(serverCtx.txcq, &(serverCtx.tx_cq_cntr), serverCtx.tx_cq_cntr + FLAGS_batch, -1);
		if (ret)
		{
			printf("SERVER: getCqCompletion failed\n\n");
			return;
		}
		// printf("Got completions: %lu\n", serverCtx.tx_cq_cntr);
	}

	if (ret)
	{
		printf("SERVER: getCqCompletion failed\n\n");
		return;
	}

	serverCtx.stopTimekeeper();
	printf("SERVER: Completed data transfer\n\n");
	PerformancePrinter::print(NULL, FLAGS_payload, FLAGS_iterations,
							  serverCtx.tx_cq_cntr, serverCtx.start, serverCtx.end, 1);
}

void startClient3()
{
	int ret;
	Client client = Client(FLAGS_provider, FLAGS_endpoint, FLAGS_dst_addr, FLAGS_dst_port);
	ret = client.init();
	if (ret)
		return;

	ConnectionContext clientCtx = client.getConnectionContext();

	clientCtx.timeout_sec = -1;

	printf("CLIENT: Receiving data transfer\n\n");
	for (int i = 1; i <= FLAGS_iterations; i++)
	{
		FabricUtil::rx(&clientCtx, clientCtx.ep, FLAGS_payload);
	}
	printf("CLIENT: Completed Receiving data transfer\n\n");
}

int main(int argc, char *argv[])
{
	initCommandLine(argc, argv);

	if (FLAGS_debug)
	{
		libefa::ENABLE_DEBUG = true;
	}

	if (FLAGS_fabinfo)
	{
		FabricInfo fabInfo = FabricInfo();
		fabInfo.initFabricInfo(FLAGS_provider, FLAGS_endpoint);
		fabInfo.printFabricInfoLong();
		return 0;
	}

	if (FLAGS_mode == "server")
	{
		startServer3();
	}

	if (FLAGS_mode == "client")
	{
		startClient3();
	}

	return 0;
}
