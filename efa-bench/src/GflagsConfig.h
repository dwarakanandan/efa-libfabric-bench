#pragma once

#include <gflags/gflags.h>

const std::string gflags_cmdline_message = "Benchmarking tool for AWS EC2's Elastic Fabric Adapter(EFA) network fabric using libfabric";
const std::string gflags_version_string = "1.0";

void initGflagsFromArgs(int argc, char *argv[]);

DECLARE_bool(fabinfo);
DECLARE_bool(debug);
DECLARE_string(mode);
DECLARE_string(benchmark_type);
DECLARE_uint32(src_port);
DECLARE_uint32(dst_port);
DECLARE_string(provider);
DECLARE_string(endpoint);
DECLARE_string(dst_addr);
DECLARE_uint32(iterations);
DECLARE_uint32(payload);
DECLARE_uint32(batch);
DECLARE_double(cq_try);
DECLARE_bool(run_all);
DECLARE_bool(tagged);