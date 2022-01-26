/*
 * Copyright (c) 2013-2017 Intel Corporation.  All rights reserved.
 * Copyright (c) 2014-2017, Cisco Systems, Inc. All rights reserved.
 *
 * This software is available to you under the BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SHARED_H_
#define _SHARED_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "includes.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef FT_FIVERSION
#define FT_FIVERSION FI_VERSION(1, 9)
#endif

#define OFI_UTIL_PREFIX "ofi_"
#define OFI_NAME_DELIM ';'

#define ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) ALIGN_MASK(x, (typeof(x))(a)-1)
#define ALIGN_DOWN(x, a) ALIGN((x) - ((a)-1), (a))

#define OFI_MR_BASIC_MAP (FI_MR_ALLOCATED | FI_MR_PROV_KEY | FI_MR_VIRT_ADDR)

	/* exit codes must be 0-255 */
	static inline int ft_exit_code(int ret)
	{
		int absret = ret < 0 ? -ret : ret;
		return absret > 255 ? EXIT_FAILURE : absret;
	}

#define ft_sa_family(addr) (((struct sockaddr *)(addr))->sa_family)

	struct test_size_param
	{
		size_t size;
		int enable_flags;
	};

	extern struct test_size_param test_size[];
	extern const unsigned int test_cnt;
#define TEST_CNT test_cnt

#define FT_ENABLE_ALL (~0)
#define FT_DEFAULT_SIZE (1 << 0)

#define MAX_XFER_SIZE (1 << 20)

	enum precision
	{
		NANO = 1,
		MICRO = 1000,
		MILLI = 1000000,
	};

	enum ft_comp_method
	{
		FT_COMP_SPIN = 0,
		FT_COMP_SREAD,
		FT_COMP_WAITSET,
		FT_COMP_WAIT_FD,
		FT_COMP_YIELD,
	};

	enum
	{
		FT_OPT_ACTIVE = 1 << 0,
		FT_OPT_ITER = 1 << 1,
		FT_OPT_SIZE = 1 << 2,
		FT_OPT_RX_CQ = 1 << 3,
		FT_OPT_TX_CQ = 1 << 4,
		FT_OPT_RX_CNTR = 1 << 5,
		FT_OPT_TX_CNTR = 1 << 6,
		FT_OPT_VERIFY_DATA = 1 << 7,
		FT_OPT_ALIGN = 1 << 8,
		FT_OPT_BW = 1 << 9,
		FT_OPT_CQ_SHARED = 1 << 10,
		FT_OPT_OOB_SYNC = 1 << 11,
		FT_OPT_SKIP_MSG_ALLOC = 1 << 12,
		FT_OPT_SKIP_REG_MR = 1 << 13,
		FT_OPT_OOB_ADDR_EXCH = 1 << 14,
		FT_OPT_ALLOC_MULT_MR = 1 << 15,
		FT_OPT_SERVER_PERSIST = 1 << 16,
		FT_OPT_ENABLE_HMEM = 1 << 17,
		FT_OPT_USE_DEVICE = 1 << 18,
		FT_OPT_DOMAIN_EQ = 1 << 19,
		FT_OPT_FORK_CHILD = 1 << 20,
		FT_OPT_SELECTIVE_COMP = 1 << 21,
		FT_OPT_INIT_LARGE_BUFFER = 1 << 22,
		FT_OPT_OOB_CTRL = FT_OPT_OOB_SYNC | FT_OPT_OOB_ADDR_EXCH,
	};

	/* for RMA tests --- we want to be able to select fi_writedata, but there is no
 * constant in libfabric for this */
	enum ft_rma_opcodes
	{
		FT_RMA_READ = 1,
		FT_RMA_WRITE,
		FT_RMA_WRITEDATA,
	};

	enum ft_atomic_opcodes
	{
		FT_ATOMIC_BASE,
		FT_ATOMIC_FETCH,
		FT_ATOMIC_COMPARE,
	};

	enum op_state
	{
		OP_DONE = 0,
		OP_PENDING
	};

	struct ft_context
	{
		char *buf;
		void *desc;
		enum op_state state;
		struct fid_mr *mr;
		struct fi_context2 context;
	};

	struct ft_opts
	{
		int iterations;
		int warmup_iterations;
		size_t transfer_size;
		int window_size;
		int av_size;
		int verbose;
		int tx_cq_size;
		int rx_cq_size;
		char *src_port;
		char *dst_port;
		char *src_addr;
		char *dst_addr;
		char *av_name;
		int sizes_enabled;
		int options;
		enum ft_comp_method comp_method;
		int machr;
		enum ft_rma_opcodes rma_op;
		char *oob_port;
		int argc;
		int num_connections;
		int address_format;

		uint64_t mr_mode;
		/* Fail if the selected provider does not support FI_MSG_PREFIX.  */
		int force_prefix;
		enum fi_hmem_iface iface;
		uint64_t device;

		char **argv;

		size_t large_bufffer_size_gbs;
	};

	struct ConnectionContext
	{
		struct fi_info *fi_pep, *fi, *hints;
		struct fid_fabric *fabric;
		struct fid_wait *waitset;
		struct fid_domain *domain;
		struct fid_poll *pollset;
		struct fid_pep *pep;
		struct fid_ep *ep, *alias_ep;
		struct fid_cq *txcq, *rxcq;
		struct fid_cntr *txcntr, *rxcntr;
		struct fid_mr *mr, no_mr;
		void *mr_desc;
		struct fid_av *av;
		struct fid_eq *eq;
		struct fid_mc *mc;

		fi_addr_t remote_fi_addr;
		char *buf, *tx_buf, *rx_buf;
		struct ft_context *tx_ctx_arr, *rx_ctx_arr;
		char **tx_mr_bufs, **rx_mr_bufs;
		size_t buf_size, tx_size, rx_size, tx_mr_size, rx_mr_size;
		int tx_fd, rx_fd;
		int timeout;

		struct fi_context tx_ctx, rx_ctx;
		uint64_t remote_cq_data;

		uint64_t tx_seq, rx_seq, tx_cq_cntr, rx_cq_cntr;
		struct fi_av_attr av_attr;
		struct fi_eq_attr eq_attr;
		struct fi_cq_attr cq_attr;
		struct fi_cntr_attr cntr_attr;

		struct fi_rma_iov remote;

		char test_name[50];
		struct timespec start, end;
		struct ft_opts opts;

		int listen_sock;
		int sock;
		int oob_sock;
		uint64_t ft_tag;

		struct fid_mr *mr_multi_recv;
		struct fi_context ctx_multi_recv[2];
		int comp_per_buf;
	};

	void init_connection_context(struct ConnectionContext *ctx);

	void ft_parseinfo(int op, char *optarg, struct fi_info *hints,
					  struct ft_opts *opts);
	void ft_parse_addr_opts(int op, char *optarg, struct ft_opts *opts);
	void ft_parsecsopts(int op, char *optarg, struct ft_opts *opts);
	int ft_parse_rma_opts(struct ConnectionContext *ctx, int op, char *optarg, struct fi_info *hints,
						  struct ft_opts *opts);
	void ft_addr_usage();
	void ft_usage(char *name, char *desc);
	void ft_mcusage(char *name, char *desc);
	void ft_csusage(char *name, char *desc);

	int ft_fill_buf(struct ConnectionContext *ctx, void *buf, size_t size);
	int ft_check_buf(struct ConnectionContext *ctx, void *buf, size_t size);
	int ft_check_opts(struct ConnectionContext *ctx, uint64_t flags);
	uint64_t ft_init_cq_data(struct fi_info *info);
	int ft_sock_listen(struct ConnectionContext *ctx, char *node, char *service);
	int ft_sock_connect(struct ConnectionContext *ctx, char *node, char *service);
	int ft_sock_accept();
	int ft_sock_send(struct ConnectionContext *ctx, int fd, void *msg, size_t len);
	int ft_sock_recv(struct ConnectionContext *ctx, int fd, void *msg, size_t len);
	int ft_sock_sync(struct ConnectionContext *ctx, int value);
	void ft_sock_shutdown(int fd);
	extern int (*ft_mr_alloc_func)(void);
	extern int ft_parent_proc;
	extern int ft_socket_pair[2];
#define ADDR_OPTS "B:P:s:a:b::E::C:F:"
#define FAB_OPTS "f:d:p:D:i:HK"
#define INFO_OPTS FAB_OPTS "e:M:"
#define CS_OPTS ADDR_OPTS "I:QS:mc:t:w:l"
#define NO_CQ_DATA 0

	extern char default_port[8];

#define FT_STR_LEN 32
#define FT_MAX_CTRL_MSG 256
#define FT_MR_KEY 0xC0DE
#define FT_TX_MR_KEY (FT_MR_KEY + 1)
#define FT_RX_MR_KEY 0xFFFF
#define FT_MSG_MR_ACCESS (FI_SEND | FI_RECV)
#define FT_RMA_MR_ACCESS (FI_READ | FI_WRITE | FI_REMOTE_READ | FI_REMOTE_WRITE)

	int ft_getsrcaddr(char *node, char *service, struct fi_info *hints);
	int ft_read_addr_opts(char **node, char **service, struct fi_info *hints,
						  uint64_t *flags, struct ft_opts *opts);
	char *size_str(char str[FT_STR_LEN], long long size);
	char *cnt_str(char str[FT_STR_LEN], long long cnt);
	int size_to_count(struct ConnectionContext *ctx, int size);
	size_t datatype_to_size(enum fi_datatype datatype);

	static inline int ft_use_size(struct ConnectionContext *ctx, int index, int enable_flags)
	{
		return test_size[index].size <= ctx->fi->ep_attr->max_msg_size &&
			   ((enable_flags == FT_ENABLE_ALL) ||
				(enable_flags & test_size[index].enable_flags));
	}

#define FT_PRINTERR(call, retv)                                              \
	do                                                                       \
	{                                                                        \
		fprintf(stderr, call "(): %s:%d, ret=%d (%s)\n", __FILE__, __LINE__, \
				(int)(retv), fi_strerror((int)-(retv)));                     \
	} while (0)

#define FT_LOG(level, fmt, ...)                                            \
	do                                                                     \
	{                                                                      \
		fprintf(stderr, "[%s] fabtests:%s:%d: " fmt "\n", level, __FILE__, \
				__LINE__, ##__VA_ARGS__);                                  \
	} while (0)

#define FT_ERR(fmt, ...) FT_LOG("error", fmt, ##__VA_ARGS__)
#define FT_WARN(fmt, ...) FT_LOG("warn", fmt, ##__VA_ARGS__)

#if ENABLE_DEBUG
#define FT_DEBUG(fmt, ...) FT_LOG("debug", fmt, ##__VA_ARGS__)
#else
#define FT_DEBUG(fmt, ...)
#endif

#define FT_EQ_ERR(eq, entry, buf, len)             \
	FT_ERR("eq_readerr (Provider errno: %d) : %s", \
		   entry.prov_errno, fi_eq_strerror(eq, entry.prov_errno, entry.err_data, buf, len))

#define FT_CQ_ERR(cq, entry, buf, len)             \
	FT_ERR("cq_readerr (Provider errno: %d) : %s", \
		   entry.prov_errno, fi_cq_strerror(cq, entry.prov_errno, entry.err_data, buf, len))

#define FT_CLOSE_FID(fd)                          \
	do                                            \
	{                                             \
		int ret;                                  \
		if ((fd))                                 \
		{                                         \
			ret = fi_close(&(fd)->fid);           \
			if (ret)                              \
				FT_ERR("fi_close: %s(%d) fid %d", \
					   fi_strerror(-ret),         \
					   ret,                       \
					   (int)(fd)->fid.fclass);    \
			fd = NULL;                            \
		}                                         \
	} while (0)

#define FT_CLOSEV_FID(fd, cnt)      \
	do                              \
	{                               \
		int i;                      \
		if (!(fd))                  \
			break;                  \
		for (i = 0; i < (cnt); i++) \
		{                           \
			FT_CLOSE_FID((fd)[i]);  \
		}                           \
	} while (0)

#define FT_EP_BIND(ep, fd, flags)                        \
	do                                                   \
	{                                                    \
		int ret;                                         \
		if ((fd))                                        \
		{                                                \
			ret = fi_ep_bind((ep), &(fd)->fid, (flags)); \
			if (ret)                                     \
			{                                            \
				FT_PRINTERR("fi_ep_bind", ret);          \
				return ret;                              \
			}                                            \
		}                                                \
	} while (0)

	void init_opts(struct ft_opts *init_opts);
	int ft_alloc_bufs();
	int ft_open_fabric_res(struct ConnectionContext *ctx);
	int ft_getinfo(struct ConnectionContext *ctx, struct fi_info *hints, struct fi_info **info);
	int ft_init_fabric(struct ConnectionContext *ctx);
	int ft_init_oob(struct ConnectionContext *ctx);
	int ft_start_server(struct ConnectionContext *ctx);
	int ft_server_connect(struct ConnectionContext *ctx);
	int ft_client_connect(struct ConnectionContext *ctx);
	int ft_init_fabric_cm(struct ConnectionContext *ctx);
	int ft_complete_connect(struct fid_ep *ep, struct fid_eq *eq);
	int ft_retrieve_conn_req(struct fid_eq *eq, struct fi_info **fi);
	int ft_accept_connection(struct fid_ep *ep, struct fid_eq *eq);
	int ft_connect_ep(struct fid_ep *ep,
					  struct fid_eq *eq, fi_addr_t *remote_addr);
	int ft_alloc_ep_res(struct ConnectionContext *ctx, struct fi_info *fi);
	int ft_alloc_active_res(struct ConnectionContext *ctx, struct fi_info *fi);
	int ft_enable_ep_recv(struct ConnectionContext *ctx);
	int ft_enable_ep(struct ConnectionContext *ctx, struct fid_ep *ep, struct fid_eq *eq, struct fid_av *av,
					 struct fid_cq *txcq, struct fid_cq *rxcq,
					 struct fid_cntr *txcntr, struct fid_cntr *rxcntr);
	int ft_init_alias_ep(struct ConnectionContext *ctx, uint64_t flags);
	int ft_av_insert(struct fid_av *av, void *addr, size_t count, fi_addr_t *fi_addr,
					 uint64_t flags, void *context);
	int ft_init_av(struct ConnectionContext *ctx);
	int ft_join_mc(struct ConnectionContext *ctx);
	int ft_init_av_dst_addr(struct ConnectionContext *ctx, struct fid_av *av_ptr, struct fid_ep *ep_ptr,
							fi_addr_t *remote_addr);
	int ft_init_av_addr(struct ConnectionContext *ctx, struct fid_av *av, struct fid_ep *ep,
						fi_addr_t *addr);
	int ft_exchange_keys(struct ConnectionContext *ctx, struct fi_rma_iov *peer_iov);
	void ft_free_res(struct ConnectionContext *ctx);
	void init_test(struct ConnectionContext *ctx, struct ft_opts *opts, char *test_name, size_t test_name_len);

	static inline void ft_start(struct ConnectionContext *ctx)
	{
		ctx->opts.options |= FT_OPT_ACTIVE;
		clock_gettime(CLOCK_MONOTONIC, &ctx->start);
	}
	static inline void ft_stop(struct ConnectionContext *ctx)
	{
		clock_gettime(CLOCK_MONOTONIC, &ctx->end);
		ctx->opts.options &= ~FT_OPT_ACTIVE;
	}

	/* Set the FI_MSG_PREFIX mode bit in the given fi_info structure and also set
 * the option bit in the given opts structure. If using ft_getinfo, it will
 * return -ENODATA if the provider clears the application requested mdoe bit.
 */
	static inline void ft_force_prefix(struct fi_info *info, struct ft_opts *opts)
	{
		info->mode |= FI_MSG_PREFIX;
		opts->force_prefix = 1;
	}

	/* If force_prefix was not requested, just continue. If it was requested,
 * return true if it was respected by the provider.
 */
	static inline bool ft_check_prefix_forced(struct fi_info *info,
											  struct ft_opts *opts)
	{
		if (opts->force_prefix)
		{
			return (info->tx_attr->mode & FI_MSG_PREFIX) &&
				   (info->rx_attr->mode & FI_MSG_PREFIX);
		}

		/* Continue if forced prefix wasn't requested. */
		return true;
	}

	int ft_sync(struct ConnectionContext *ctx);
	int ft_sync_pair(int status);
	int ft_fork_and_pair(void);
	int ft_fork_child(void);
	int ft_wait_child(void);
	int ft_finalize(struct ConnectionContext *ctx);
	int ft_finalize_ep(struct ConnectionContext *ctx, struct fid_ep *ep);

	size_t ft_rx_prefix_size(struct ConnectionContext *ctx);
	size_t ft_tx_prefix_size(struct ConnectionContext *ctx);
	ssize_t ft_post_rx(struct ConnectionContext *ctx, struct fid_ep *ep, size_t size, void *ctxptr);
	ssize_t ft_post_rx_buf(struct ConnectionContext *ctx, struct fid_ep *ep, size_t size, void *ctxptr,
						   void *op_buf, void *op_mr_desc, uint64_t op_tag);
	ssize_t ft_post_tx(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr, size_t size,
					   uint64_t data, void *ctxptr);
	ssize_t ft_post_tx_buf(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr, size_t size,
						   uint64_t data, void *ctxptr,
						   void *op_buf, void *op_mr_desc, uint64_t op_tag);
	ssize_t ft_rx(struct ConnectionContext *ctx, struct fid_ep *ep, size_t size);
	ssize_t ft_tx(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr, size_t size, void *ctxptr);
	ssize_t ft_inject(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr, size_t size);
	ssize_t ft_post_rma(struct ConnectionContext *ctx, enum ft_rma_opcodes op, struct fid_ep *ep, size_t size,
						struct fi_rma_iov *remote, void *context);
	ssize_t ft_rma(struct ConnectionContext *ctx, enum ft_rma_opcodes op, struct fid_ep *ep, size_t size,
				   struct fi_rma_iov *remote, void *context);
	ssize_t ft_post_rma_inject(struct ConnectionContext *ctx, enum ft_rma_opcodes op, struct fid_ep *ep, size_t size,
							   struct fi_rma_iov *remote);

	ssize_t ft_post_atomic(struct ConnectionContext *ctx, enum ft_atomic_opcodes opcode, struct fid_ep *ep,
						   void *compare, void *compare_desc, void *result,
						   void *result_desc, struct fi_rma_iov *remote,
						   enum fi_datatype datatype, enum fi_op atomic_op,
						   void *context);
	int check_base_atomic_op(struct ConnectionContext *ctx, struct fid_ep *endpoint, enum fi_op op,
							 enum fi_datatype datatype, size_t *count);
	int check_fetch_atomic_op(struct ConnectionContext *ctx, struct fid_ep *endpoint, enum fi_op op,
							  enum fi_datatype datatype, size_t *count);
	int check_compare_atomic_op(struct ConnectionContext *ctx, struct fid_ep *endpoint, enum fi_op op,
								enum fi_datatype datatype, size_t *count);

	int ft_cq_readerr(struct fid_cq *cq);
	int ft_get_rx_comp(struct ConnectionContext *ctx, uint64_t total);
	int ft_get_tx_comp(struct ConnectionContext *ctx, uint64_t total);
	int ft_recvmsg(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr,
				   size_t size, void *ctxptr, int flags);
	int ft_sendmsg(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr,
				   size_t size, void *ctxptr, int flags);
	int ft_cq_read_verify(struct ConnectionContext *ctx, struct fid_cq *cq, void *op_context);

	void eq_readerr(struct fid_eq *eq, const char *eq_str);

	int64_t get_elapsed(const struct timespec *b, const struct timespec *a,
						enum precision p);
	void show_perf(char *name, size_t tsize, int iters, struct timespec *start,
				   struct timespec *end, int xfers_per_iter);
	void show_perf_mr(size_t tsize, int iters, struct timespec *start,
					  struct timespec *end, int xfers_per_iter, int argc, char *argv[]);

	int ft_send_recv_greeting(struct ConnectionContext *ctx, struct fid_ep *ep);
	int ft_send_greeting(struct ConnectionContext *ctx, struct fid_ep *ep);
	int ft_recv_greeting(struct ConnectionContext *ctx, struct fid_ep *ep);

	int ft_accept_next_client();

	int check_recv_msg(struct ConnectionContext *ctx, const char *message);
	uint64_t ft_info_to_mr_access(struct fi_info *info);
	int ft_alloc_bit_combo(uint64_t fixed, uint64_t opt, uint64_t **combos, int *len);
	void ft_free_bit_combo(uint64_t *combo);
	int ft_cntr_open(struct ConnectionContext *ctx, struct fid_cntr **cntr);
	const char *ft_util_name(const char *str, size_t *len);
	const char *ft_core_name(const char *str, size_t *len);
	char **ft_split_and_alloc(const char *s, const char *delim, size_t *count);
	void ft_free_string_array(char **s);

	ssize_t ft_post_rma_selective_comp(struct ConnectionContext *ctx, enum ft_rma_opcodes op, struct fid_ep *ep, size_t size,
									   struct fi_rma_iov *remote, void *context, bool enable_completion);

	int alloc_ep_res_multi_recv(struct ConnectionContext *ctx);
	int repost_multi_recv(struct ConnectionContext *ctx, int chunk);
	int wait_for_multi_recv_completion(struct ConnectionContext *ctx, int num_completions);

#define FT_PROCESS_QUEUE_ERR(readerr, rd, queue, fn, str) \
	do                                                    \
	{                                                     \
		if (rd == -FI_EAVAIL)                             \
		{                                                 \
			readerr(queue, fn " " str);                   \
		}                                                 \
		else                                              \
		{                                                 \
			FT_PRINTERR(fn, rd);                          \
		}                                                 \
	} while (0)

#define FT_PROCESS_EQ_ERR(rd, eq, fn, str) \
	FT_PROCESS_QUEUE_ERR(eq_readerr, rd, eq, fn, str)

#define FT_OPTS_USAGE_FORMAT "%-30s %s"
#define FT_PRINT_OPTS_USAGE(opt, desc) fprintf(stderr, FT_OPTS_USAGE_FORMAT "\n", opt, desc)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ARRAY_SIZE(A) (sizeof(A) / sizeof(*A))

#define TEST_ENUM_SET_N_RETURN(str, len, enum_val, type, data) \
	TEST_SET_N_RETURN(str, len, #enum_val, enum_val, type, data)

#define TEST_SET_N_RETURN(str, len, val_str, val, type, data) \
	do                                                        \
	{                                                         \
		if (!strncmp(str, val_str, len))                      \
		{                                                     \
			*(type *)(data) = val;                            \
			return 0;                                         \
		}                                                     \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* _SHARED_H_ */
