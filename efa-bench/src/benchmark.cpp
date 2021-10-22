#include <iostream>

#include <libefa/FabricInfo.h>
#include "libefa/server.h"
#include "libefa/client.h"

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

int main(int argc, char *argv[])
{
	init_command_line(argc, argv);

	if (FLAGS_fabinfo)
	{
		FabricInfo fabInfo = FabricInfo();
		fabInfo.loadFabricInfo(FLAGS_provider);
		fabInfo.printFabricInfoShort();
		return 0;
	}

	if (FLAGS_mode == "server")
	{
		start_server();
	}

	if (FLAGS_mode == "client")
	{
		start_client();
	}

	return 0;
}
