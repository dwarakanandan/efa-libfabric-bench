#pragma once

#include "../includes.h"

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

#define PP_EP_BIND(ep, fd, flags)                        \
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

#define PP_POST(post_fn, comp_fn, seq, op_str, ...)            \
    do                                                         \
    {                                                          \
        int timeout_sec_save;                                  \
        int ret, rc;                                           \
                                                               \
        while (1)                                              \
        {                                                      \
            ret = post_fn(__VA_ARGS__);                        \
            if (!ret)                                          \
                break;                                         \
                                                               \
            if (ret != -FI_EAGAIN)                             \
            {                                                  \
                PRINTERR(op_str, ret);                         \
                return ret;                                    \
            }                                                  \
                                                               \
            timeout_sec_save = ct->timeout_sec;                \
            ct->timeout_sec = 0;                               \
            rc = comp_fn(ct, seq);                             \
            ct->timeout_sec = timeout_sec_save;                \
            if (rc && rc != -FI_EAGAIN)                        \
            {                                                  \
                printf("Failed to get " op_str " completion"); \
                return rc;                                     \
            }                                                  \
        }                                                      \
        seq++;                                                 \
    } while (0)

#define INTEG_SEED 7
static const char integ_alphabet[] =
	"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
/* Size does not include trailing new line */
static const int integ_alphabet_length =
	(sizeof(integ_alphabet) / sizeof(*integ_alphabet)) - 1;


void DEBUG(std::string str);

ssize_t pp_post_rx(struct ctx_connection *ct, struct fid_ep *ep, size_t size, void *ctx);

int pp_get_rx_comp(struct ctx_connection *ct, uint64_t total);

int pp_alloc_msgs(struct ctx_connection *ct);

int pp_ctrl_send(struct ctx_connection *ct, char *buf, size_t size);

int pp_ctrl_recv(struct ctx_connection *ct, char *buf, size_t size);

int pp_ctrl_recv_str(struct ctx_connection *ct, char *buf, size_t size);

int open_fabric_res(struct ctx_connection *ct);

int alloc_active_res(struct ctx_connection *ct, struct fi_info *fi);

int init_ep(struct ctx_connection *ct);

int send_name(struct ctx_connection *ct, struct fid *endpoint);

int recv_name(struct ctx_connection *ct);

int av_insert(struct fid_av *av, void *addr, size_t count, fi_addr_t *fi_addr, uint64_t flags, void *context);

uint64_t pp_gettime_us(void);

ssize_t pp_rx(struct ctx_connection *ct, struct fid_ep *ep, size_t size);

ssize_t pp_inject(struct ctx_connection *ct, struct fid_ep *ep, size_t size);

ssize_t pp_tx(struct ctx_connection *ct, struct fid_ep *ep, size_t size);

char *size_str(char *str, uint64_t size);

char *cnt_str(char *str, size_t size, uint64_t cnt);

long parse_ulong(char *str, long max);

void show_perf(char *name, int tsize, int sent, int acked, uint64_t start, uint64_t end, int xfers_per_iter);