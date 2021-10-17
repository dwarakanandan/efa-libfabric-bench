#pragma once

#include <gflags/gflags.h>

const std::string gflags_cmdline_message = "Benchmarking tool for AWS EC2's Elastic Fabric Adapter(EFA) network fabric using libfabric";
const std::string gflags_version_string = "1.0";

DECLARE_bool(fiinfo);
DECLARE_bool(debug);
DECLARE_string(mode);
DECLARE_uint32(src_port);
DECLARE_uint32(dst_port);
DECLARE_string(provider);
DECLARE_string(dst_addr);
DECLARE_uint32(iterations);
DECLARE_uint32(payload_size);
