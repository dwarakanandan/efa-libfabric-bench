#include "includes.h"
#include "cmd_line.h"
#include "./fi_pingpong_util/fi_pingpong_util.h"

int fabric_getinfo(struct ctx_connection *ct, struct fi_info *hints, struct fi_info **info);

void print_long_info(struct fi_info *info);

void print_short_info(struct fi_info *info);

void generate_hints(struct fi_info **info);

void chrono_start(struct ctx_connection *ct);

void chrono_stop(struct ctx_connection *ct);

void banner_fabric_info(struct ctx_connection *ct);

void show_perf(char *name, int tsize, int sent, int acked, uint64_t start, uint64_t end, int xfers_per_iter);


