#pragma once

#include <gflags/gflags.h>

const std::string gflags_cmdline_message = "Benchmarking tool for AWS EC2's Elastic Fabric Adapter(EFA) network fabric using libfabric";
const std::string gflags_version_string = "1.0";

DEFINE_bool(fi_info, false, "Show module info");