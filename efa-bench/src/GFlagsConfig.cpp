#include "GflagsConfig.h"

void initGflagsFromArgs(int argc, char *argv[])
{
	gflags::SetUsageMessage(gflags_cmdline_message);
	gflags::SetVersionString(gflags_version_string);

	if (argc == 1)
	{
		gflags::ShowUsageWithFlagsRestrict(argv[0], "GFlagsConfig");
		exit(0);
	}

	gflags::ParseCommandLineFlags(&argc, &argv, true);
}

DEFINE_bool(fabinfo, false, "Show provider info");
DEFINE_bool(debug, false, "Print debug logs");
DEFINE_string(mode, "server", "Mode of operation Eg: server, client");
DEFINE_string(benchmark_type, "batch", "Type of benchmark Eg: batch, latency, inject, ping_pong, rma, rma_batch, rma_sel_comp");
DEFINE_uint32(src_port, 47500, "Source port server listens on");
DEFINE_uint32(dst_port, 47500, "Destination port client connects to");
DEFINE_string(provider, "sockets", "Fabric provider Eg: sockets, efa");
DEFINE_string(endpoint, "dgram", "Endpoint type Eg: dgram, rdm");
DEFINE_string(dst_addr, "127.0.0.1", "Destination address");
DEFINE_uint32(iterations, 10, "Number of transfers to perform");
DEFINE_uint32(payload, 64, "Size of transfer payload in Kilobytes");
DEFINE_uint32(batch, 1000, "Batch size");
DEFINE_double(cq_try, 0.8, "Factor used in combination with batch size to determine CQ retrievals");
DEFINE_bool(run_all, false, "Run benchmark for all payloads");
DEFINE_bool(tagged, false, "Use Tagged message transfer");
DEFINE_string(rma_op, "write", "RMA Operation Eg: read/write");