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
DEFINE_string(benchmark_type, "batch", "Type of benchmark Eg: batch, latency, inject, ping_pong, rma, rma_batch, rma_inject, rma_sel_comp, rma_large_buffer, sat_latency");
DEFINE_uint32(port, 47500, "Port for data transfer");
DEFINE_uint32(oob_port, 46500, "Port for out of band sync/exchange");
DEFINE_string(provider, "sockets", "Fabric provider Eg: sockets, efa");
DEFINE_string(endpoint, "dgram", "Endpoint type Eg: dgram, rdm");
DEFINE_string(dst_addr, "127.0.0.1", "Destination address");
DEFINE_uint32(iterations, 10, "Number of transfers to perform");
DEFINE_uint32(runtime, 30, "Benchmark run-time in seconds");
DEFINE_uint32(payload, 64, "Size of transfer payload in Kilobytes");
DEFINE_uint32(batch, 1000, "Batch size");
DEFINE_double(cq_try, 0.8, "Factor used in combination with batch size to determine CQ retrievals");
DEFINE_bool(run_all, false, "Run benchmark for all payloads");
DEFINE_bool(tagged, false, "Use Tagged message transfer");
DEFINE_string(rma_op, "write", "RMA Operation Eg: read/write");
DEFINE_string(hw_counters, "/sys/class/infiniband/rdmap0s6/ports/1/hw_counters/", "Path to network interface HW Counters");
DEFINE_string(stat_file, "stat-file", "Output stats file name");
DEFINE_uint32(threads, 1, "Thread count / Num of connection streams");
DEFINE_uint32(saturation_wait, 10000, "Wait interval between iterations in nano seconds");