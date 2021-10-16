#include "includes.h"
#include "cmd_line.h"
#include "fi_pingpong_util.h"

void DEBUG(std::string str);

int fabric_getinfo(struct ctx_connection *ct, struct fi_info *hints, struct fi_info **info);

void print_long_info(struct fi_info *info);

void print_short_info(struct fi_info *info);

void generate_hints(struct fi_info **info);

int open_fabric_res(struct ctx_connection *ct);

int alloc_active_res(struct ctx_connection *ct, struct fi_info *fi);

int init_ep(struct ctx_connection *ct);

int send_name(struct ctx_connection *ct, struct fid *endpoint);

int recv_name(struct ctx_connection *ct);

int av_insert(struct fid_av *av, void *addr, size_t count, fi_addr_t *fi_addr, uint64_t flags, void *context);
