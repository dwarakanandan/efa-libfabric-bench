#include <iostream>

#include <libefa/FabricInfo.h>
#include <libefa/Server.h>
#include <libefa/Client.h>
#include <libefa/PerformancePrinter.h>

#include "cmd_line.h"

using namespace std;
using namespace libefa;

void init_command_line(int argc, char *argv[])
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
		ret = FabricUtil::rx(&serverCtx, serverCtx.ep, FLAGS_payload_size);
		if (ret)
			return;
		ret = FabricUtil::tx(&serverCtx, serverCtx.ep, FLAGS_payload_size);
		if (ret)
			return;
	}

	serverCtx.stopTimekeeper();
	printf("SERVER: Completed data transfer\n\n");

	PerformancePrinter::print(NULL, FLAGS_payload_size, FLAGS_iterations,
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
		ret = FabricUtil::tx(&clientCtx, clientCtx.ep, FLAGS_payload_size);
		if (ret)
			return;
		ret = FabricUtil::rx(&clientCtx, clientCtx.ep, FLAGS_payload_size);
		if (ret)
			return;
	}

	clientCtx.stopTimekeeper();
	printf("CLIENT: Completed data transfer\n\n");

	PerformancePrinter::print(NULL, FLAGS_payload_size, FLAGS_iterations,
							  clientCtx.cnt_ack_msg, clientCtx.start, clientCtx.end, 2);
}

int main(int argc, char *argv[])
{
	init_command_line(argc, argv);

	if (FLAGS_debug)
	{
		libefa::ENABLE_DEBUG = true;
	}

	if (FLAGS_fabinfo)
	{
		FabricInfo fabInfo = FabricInfo();
		fabInfo.initFabricInfo(FLAGS_provider, "dgram");
		fabInfo.printFabricInfoShort();
		return 0;
	}

	if (FLAGS_mode == "server")
	{
		startServer();
	}

	if (FLAGS_mode == "client")
	{
		startClient();
	}

	return 0;
}
