#include <iostream>

#include "cmd_line.h"
#include "fi_info.h"

using namespace std;

int main(int argc, char *argv[])
{
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	if (FLAGS_fi_info) {
		print_fi_info();
		return 0;
	}

	return 0;
}
