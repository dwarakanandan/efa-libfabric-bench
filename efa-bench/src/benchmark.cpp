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
	FabricUtil::tx(&serverCtx, serverCtx.ep, FLAGS_payload);
	printf("SERVER: Completed data transfer\n\n");
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
	FabricUtil::rx(&clientCtx, clientCtx.ep, FLAGS_payload);
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
