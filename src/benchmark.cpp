#include <iostream>

#include "cmd_line.h"
#include "fi_info.h"
#include "server.h"
#include "client.h"

using namespace std;

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

	if (FLAGS_fiinfo)
	{
		print_fi_info();
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
