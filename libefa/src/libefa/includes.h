#pragma once

#include <iostream>

#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <getopt.h>
#include <inttypes.h>
#include <netdb.h>
#include <poll.h>
#include <limits.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_tagged.h>

#define SOCKET int

#define CTRL_BUF_LEN 64
#define MSG_SYNC_Q "q"
#define MSG_SYNC_A "a"
#define MSG_CHECK_CNT_OK "cnt ok"
#define MSG_LEN_CNT 10

#define PP_CTRL_BUF_LEN 64
#define PP_SIZE_MAX_POWER_TWO 22
#define PP_MAX_DATA_MSG \
    ((1 << PP_SIZE_MAX_POWER_TWO) + (1 << (PP_SIZE_MAX_POWER_TWO - 1)))
#define PP_MAX_CTRL_MSG 64
#define PP_CTRL_BUF_LEN 64
#define PP_MR_KEY 0xC0DE
#define PP_MAX_ADDRLEN 1024
#define PP_STR_LEN 32

static const uint64_t TAG = 1234;

void DEBUG(std::string str);

namespace libefa
{
    extern bool ENABLE_DEBUG;
    extern bool ENABLE_RXTX_VERIFICATION;
}

#define PRINTERR(call, retv)                                        \
    fprintf(stderr, "%s(): %s:%-4d, ret=%d (%s)\n", call, __FILE__, \
            __LINE__, (int)retv, fi_strerror((int)-retv))

#define MAX(a, b)               \
    (                           \
        {                       \
            typeof(a) _a = (a); \
            typeof(b) _b = (b); \
            _a > _b ? _a : _b;  \
        })

#define EP_BIND(ep, fd, flags)                           \
    do                                                   \
    {                                                    \
        int ret;                                         \
        if ((fd))                                        \
        {                                                \
            ret = fi_ep_bind((ep), &(fd)->fid, (flags)); \
            if (ret)                                     \
            {                                            \
                PRINTERR("fi_ep_bind", ret);             \
                return ret;                              \
            }                                            \
        }                                                \
    } while (0)

#define INTEG_SEED 7
static const char integ_alphabet[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
/* Size does not include trailing new line */
static const int integ_alphabet_length =
    (sizeof(integ_alphabet) / sizeof(*integ_alphabet)) - 1;
