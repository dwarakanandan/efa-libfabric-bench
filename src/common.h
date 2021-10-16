#include "includes.h"
#include "cmd_line.h"
#include "fi_pingpong_util.h"

void DEBUG(std::string str);

int fabric_getinfo(struct ctx_connection *ct, struct fi_info *hints, struct fi_info **info);

void print_long_info(struct fi_info *info);

void print_short_info(struct fi_info *info);

void generate_hints(struct fi_info **info);


