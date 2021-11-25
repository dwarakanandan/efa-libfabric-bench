#include "Common.h"

bool common::is_benchmark(std::string t1, T t2) 
{
    return t1.compare(BENCHMARK_TYPE[t2]) == 0;
}

bool common::is_node(std::string t1, T t2)
{
    return t1.compare(NODE_TYPE[t2]) == 0;
}