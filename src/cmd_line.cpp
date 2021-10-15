#include "cmd_line.h"

DEFINE_bool(fiinfo, false, "Show provider info");
DEFINE_bool(debug, false, "Print debug logs");
DEFINE_string(mode, "server", "Mode of operation Eg: server,client");
DEFINE_uint32(src_port, 47500, "Source port");
DEFINE_uint32(dst_port, 47500, "Destination port");
DEFINE_string(provider, "sockets", "Fabric provider Eg: sockets,efa");
DEFINE_string(dst_addr, "127.0.0.1", "Destination address");