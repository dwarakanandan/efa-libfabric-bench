/*
 * Copyright (c) 2013-2018 Intel Corporation.  All rights reserved.
 * Copyright (c) 2016 Cray Inc.  All rights reserved.
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

#include "shared.h"
#include "hmem.h"

int (*ft_mr_alloc_func)(void);

int ft_parent_proc = 0;
pid_t ft_child_pid = 0;
int ft_socket_pair[2];

char default_port[8] = "9228";
static char default_oob_port[8] = "3000";
const char *greeting = "Hello from Client!";

char test_name[50] = "custom";

void init_connection_context(struct ConnectionContext *ctx)
{
	ctx->mr_desc = NULL;
	ctx->tx_ctx_arr = NULL;
	ctx->rx_ctx_arr = NULL;
	ctx->remote_cq_data = 0;

	ctx->remote_fi_addr = FI_ADDR_UNSPEC;
	ctx->tx_mr_bufs = NULL;
	ctx->rx_mr_bufs = NULL;
	ctx->rx_fd = -1;
	ctx->tx_fd = -1;

	ctx->timeout = -1;
	ctx->av_attr.type = FI_AV_MAP;
	ctx->av_attr.count = 1;

	ctx->eq_attr.wait_obj = FI_WAIT_UNSPEC;
	ctx->cq_attr.wait_obj = FI_WAIT_NONE;
	ctx->cntr_attr.events = FI_CNTR_EVENTS_COMP;
	ctx->cntr_attr.wait_obj = FI_WAIT_NONE;

	ctx->listen_sock = -1;
	ctx->sock = -1;
	ctx->oob_sock = -1;
}

struct test_size_param test_size[] = {
	{1 << 0, 0},
	{1 << 1, 0},
	{(1 << 1) + (1 << 0), 0},
	{1 << 2, 0},
	{(1 << 2) + (1 << 1), 0},
	{1 << 3, 0},
	{(1 << 3) + (1 << 2), 0},
	{1 << 4, 0},
	{(1 << 4) + (1 << 3), 0},
	{1 << 5, 0},
	{(1 << 5) + (1 << 4), 0},
	{1 << 6, FT_DEFAULT_SIZE},
	{(1 << 6) + (1 << 5), 0},
	{1 << 7, 0},
	{(1 << 7) + (1 << 6), 0},
	{1 << 8, FT_DEFAULT_SIZE},
	{(1 << 8) + (1 << 7), 0},
	{1 << 9, 0},
	{(1 << 9) + (1 << 8), 0},
	{1 << 10, FT_DEFAULT_SIZE},
	{(1 << 10) + (1 << 9), 0},
	{1 << 11, 0},
	{(1 << 11) + (1 << 10), 0},
	{1 << 12, FT_DEFAULT_SIZE},
	{(1 << 12) + (1 << 11), 0},
	{1 << 13, 0},
	{(1 << 13) + (1 << 12), 0},
	{1 << 14, 0},
	{(1 << 14) + (1 << 13), 0},
	{1 << 15, 0},
	{(1 << 15) + (1 << 14), 0},
	{1 << 16, FT_DEFAULT_SIZE},
	{(1 << 16) + (1 << 15), 0},
	{1 << 17, 0},
	{(1 << 17) + (1 << 16), 0},
	{1 << 18, 0},
	{(1 << 18) + (1 << 17), 0},
	{1 << 19, 0},
	{(1 << 19) + (1 << 18), 0},
	{1 << 20, FT_DEFAULT_SIZE},
	{(1 << 20) + (1 << 19), 0},
	{1 << 21, 0},
	{(1 << 21) + (1 << 20), 0},
	{1 << 22, 0},
	{(1 << 22) + (1 << 21), 0},
	{1 << 23, 0},
};

const unsigned int test_cnt = (sizeof test_size / sizeof test_size[0]);

static const char integ_alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const int integ_alphabet_length = (sizeof(integ_alphabet) / sizeof(*integ_alphabet)) - 1;

static int ft_poll_fd(int fd, int timeout)
{
	struct pollfd fds;
	int ret;

	fds.fd = fd;
	fds.events = POLLIN;
	ret = poll(&fds, 1, timeout);
	if (ret == -1)
	{
		FT_PRINTERR("poll", -errno);
		ret = -errno;
	}
	else if (!ret)
	{
		ret = -FI_EAGAIN;
	}
	else
	{
		ret = 0;
	}
	return ret;
}

void init_opts(struct ft_opts *init_opts)
{
	init_opts->iterations = 1000;
	init_opts->warmup_iterations = 10;
	init_opts->transfer_size = 1024;
	init_opts->window_size = 64;
	init_opts->av_size = 1;
	init_opts->tx_cq_size = 0;
	init_opts->rx_cq_size = 0;
	init_opts->verbose = 0;
	init_opts->sizes_enabled = FT_DEFAULT_SIZE;
	init_opts->rma_op = FT_RMA_WRITE;
	init_opts->options = FT_OPT_RX_CQ | FT_OPT_TX_CQ;
	init_opts->oob_port = NULL;
	init_opts->mr_mode = FI_MR_LOCAL | OFI_MR_BASIC_MAP;
	init_opts->iface = FI_HMEM_SYSTEM;
	init_opts->device = 0;
	init_opts->address_format = FI_FORMAT_UNSPEC;
}

size_t ft_tx_prefix_size(struct ConnectionContext *ctx)
{
	return (ctx->fi->tx_attr->mode & FI_MSG_PREFIX) ? ctx->fi->ep_attr->msg_prefix_size : 0;
}

size_t ft_rx_prefix_size(struct ConnectionContext *ctx)
{
	return (ctx->fi->rx_attr->mode & FI_MSG_PREFIX) ? ctx->fi->ep_attr->msg_prefix_size : 0;
}

int ft_check_opts(struct ConnectionContext *ctx, uint64_t flags)
{
	return (ctx->opts.options & flags) == flags;
}

static void ft_cq_set_wait_attr(struct ConnectionContext *ctx)
{
	switch (ctx->opts.comp_method)
	{
	case FT_COMP_SREAD:
		ctx->cq_attr.wait_obj = FI_WAIT_UNSPEC;
		ctx->cq_attr.wait_cond = FI_CQ_COND_NONE;
		break;
	case FT_COMP_WAITSET:
		assert(ctx->waitset);
		ctx->cq_attr.wait_obj = FI_WAIT_SET;
		ctx->cq_attr.wait_cond = FI_CQ_COND_NONE;
		ctx->cq_attr.wait_set = ctx->waitset;
		break;
	case FT_COMP_WAIT_FD:
		ctx->cq_attr.wait_obj = FI_WAIT_FD;
		ctx->cq_attr.wait_cond = FI_CQ_COND_NONE;
		break;
	case FT_COMP_YIELD:
		ctx->cq_attr.wait_obj = FI_WAIT_YIELD;
		ctx->cq_attr.wait_cond = FI_CQ_COND_NONE;
		break;
	default:
		ctx->cq_attr.wait_obj = FI_WAIT_NONE;
		break;
	}
}

static void ft_cntr_set_wait_attr(struct ConnectionContext *ctx)
{
	switch (ctx->opts.comp_method)
	{
	case FT_COMP_SREAD:
		ctx->cntr_attr.wait_obj = FI_WAIT_UNSPEC;
		break;
	case FT_COMP_WAITSET:
		assert(ctx->waitset);
		ctx->cntr_attr.wait_obj = FI_WAIT_SET;
		break;
	case FT_COMP_WAIT_FD:
		ctx->cntr_attr.wait_obj = FI_WAIT_FD;
		break;
	case FT_COMP_YIELD:
		ctx->cntr_attr.wait_obj = FI_WAIT_YIELD;
		break;
	default:
		ctx->cntr_attr.wait_obj = FI_WAIT_NONE;
		break;
	}
}

int ft_cntr_open(struct ConnectionContext *ctx, struct fid_cntr **cntr)
{
	ft_cntr_set_wait_attr(ctx);
	return fi_cntr_open(ctx->domain, &ctx->cntr_attr, cntr, cntr);
}

static inline int ft_rma_read_target_allowed(uint64_t caps)
{
	if (caps & (FI_RMA | FI_ATOMIC))
	{
		if (caps & FI_REMOTE_READ)
			return 1;
		return !(caps & (FI_READ | FI_WRITE | FI_REMOTE_WRITE));
	}
	return 0;
}

static inline int ft_rma_write_target_allowed(uint64_t caps)
{
	if (caps & (FI_RMA | FI_ATOMIC))
	{
		if (caps & FI_REMOTE_WRITE)
			return 1;
		return !(caps & (FI_READ | FI_WRITE | FI_REMOTE_WRITE));
	}
	return 0;
}

static inline int ft_check_mr_local_flag(struct fi_info *info)
{
	return ((info->mode & FI_LOCAL_MR) ||
			(info->domain_attr->mr_mode & FI_MR_LOCAL));
}

uint64_t ft_info_to_mr_access(struct fi_info *info)
{
	uint64_t mr_access = 0;
	if (ft_check_mr_local_flag(info))
	{
		if (info->caps & (FI_MSG | FI_TAGGED))
		{
			if (info->caps & FT_MSG_MR_ACCESS)
			{
				mr_access |= info->caps & FT_MSG_MR_ACCESS;
			}
			else
			{
				mr_access |= FT_MSG_MR_ACCESS;
			}
		}

		if (info->caps & (FI_RMA | FI_ATOMIC))
		{
			if (info->caps & FT_RMA_MR_ACCESS)
			{
				mr_access |= info->caps & FT_RMA_MR_ACCESS;
			}
			else
			{
				mr_access |= FT_RMA_MR_ACCESS;
			}
		}
	}
	else
	{
		if (info->caps & (FI_RMA | FI_ATOMIC))
		{
			if (ft_rma_read_target_allowed(info->caps))
			{
				mr_access |= FI_REMOTE_READ;
			}
			if (ft_rma_write_target_allowed(info->caps))
			{
				mr_access |= FI_REMOTE_WRITE;
			}
		}
	}
	return mr_access;
}

#define bit_isset(x, i) (x >> i & 1)
#define for_each_bit(x, i) for (i = 0; i < (8 * sizeof(x)); i++)

static inline int bit_set_count(uint64_t val)
{
	int cnt = 0;
	while (val)
	{
		cnt++;
		val &= val - 1;
	}
	return cnt;
}

int ft_alloc_bit_combo(uint64_t fixed, uint64_t opt,
					   uint64_t **combos, int *len)
{
	uint64_t *flags;
	int i, num_flags;
	uint64_t index;
	int ret;

	num_flags = bit_set_count(opt) + 1;
	flags = calloc(num_flags, sizeof(fixed));
	if (!flags)
	{
		perror("calloc");
		return -FI_ENOMEM;
	}

	*len = 1 << (num_flags - 1);
	*combos = calloc(*len, sizeof(fixed));
	if (!(*combos))
	{
		perror("calloc");
		ret = -FI_ENOMEM;
		goto clean;
	}

	num_flags = 0;
	for_each_bit(opt, i)
	{
		if (bit_isset(opt, i))
			flags[num_flags++] = 1ULL << i;
	}

	for (index = 0; index < (*len); index++)
	{
		(*combos)[index] = fixed;
		for_each_bit(index, i)
		{
			if (bit_isset(index, i))
				(*combos)[index] |= flags[i];
		}
	}
	ret = FI_SUCCESS;

clean:
	free(flags);
	return ret;
}

void ft_free_bit_combo(uint64_t *combo)
{
	free(combo);
}

static int ft_reg_mr(struct ConnectionContext *ctx, void *buf, size_t size, uint64_t access,
					 uint64_t key, struct fid_mr **mr, void **desc)
{
	struct fi_mr_attr attr = {0};
	struct iovec iov = {0};
	int ret;

	if (((!(ctx->fi->domain_attr->mr_mode & FI_MR_LOCAL) &&
		  !(ctx->opts.options & FT_OPT_USE_DEVICE)) ||
		 (!(ctx->fi->domain_attr->mr_mode & FI_MR_HMEM) &&
		  ctx->opts.options & FT_OPT_USE_DEVICE)) &&
		!(ctx->fi->caps & (FI_RMA | FI_ATOMIC)))
		return 0;

	iov.iov_base = buf;
	iov.iov_len = size;
	attr.mr_iov = &iov;
	attr.iov_count = 1;
	attr.access = access;
	attr.offset = 0;
	attr.requested_key = key;
	attr.context = NULL;
	attr.iface = ctx->opts.iface;

	switch (ctx->opts.iface)
	{
	case FI_HMEM_ZE:
		attr.device.ze = ctx->opts.device;
		break;
	default:
		break;
	}

	ret = fi_mr_regattr(ctx->domain, &attr, 0, mr);
	if (ret)
		return ret;

	if (desc)
		*desc = fi_mr_desc(*mr);

	return FI_SUCCESS;
}

static int ft_alloc_ctx_array(struct ConnectionContext *ctx, struct ft_context **mr_array, char ***mr_bufs,
							  char *default_buf, size_t mr_size,
							  uint64_t start_key)
{
	int i, ret;
	uint64_t access = ft_info_to_mr_access(ctx->fi);
	struct ft_context *context;

	*mr_array = calloc(ctx->opts.window_size, sizeof(**mr_array));
	if (!*mr_array)
		return -FI_ENOMEM;

	if (ctx->opts.options & FT_OPT_ALLOC_MULT_MR)
	{
		*mr_bufs = calloc(ctx->opts.window_size, sizeof(**mr_bufs));
		if (!mr_bufs)
			return -FI_ENOMEM;
	}

	for (i = 0; i < ctx->opts.window_size; i++)
	{
		context = &(*mr_array)[i];
		if (!(ctx->opts.options & FT_OPT_ALLOC_MULT_MR))
		{
			context->buf = default_buf + mr_size * i;
			context->mr = ctx->mr;
			context->desc = ctx->mr_desc;
			continue;
		}
		ret = ft_hmem_alloc(ctx->opts.iface, ctx->opts.device,
							(void **)&((*mr_bufs)[i]), mr_size);
		if (ret)
			return ret;

		context->buf = (*mr_bufs)[i];

		ret = ft_reg_mr(ctx, context->buf, mr_size, access,
						start_key + i, &context->mr,
						&context->desc);
		if (ret)
			return ret;
	}

	return 0;
}

static void ft_set_tx_rx_sizes(struct ConnectionContext *ctx, size_t *set_tx, size_t *set_rx)
{
	*set_tx = ctx->opts.options & FT_OPT_SIZE ? ctx->opts.transfer_size : test_size[TEST_CNT - 1].size;
	if (*set_tx > ctx->fi->ep_attr->max_msg_size)
		*set_tx = ctx->fi->ep_attr->max_msg_size;
	*set_rx = *set_tx + ft_rx_prefix_size(ctx);
	*set_tx += ft_tx_prefix_size(ctx);
}

/*
 * Include FI_MSG_PREFIX space in the allocated buffer, and ensure that the
 * buffer is large enough for a control message used to exchange addressing
 * data.
 */
static int ft_alloc_msgs(struct ConnectionContext *ctx)
{
	int ret;
	long alignment = 1;

	if (ft_check_opts(ctx, FT_OPT_SKIP_MSG_ALLOC))
		return 0;

	if (ctx->opts.options & FT_OPT_ALLOC_MULT_MR)
	{
		ft_set_tx_rx_sizes(ctx, &ctx->tx_mr_size, &ctx->rx_mr_size);
		ctx->rx_size = FT_MAX_CTRL_MSG + ft_rx_prefix_size(ctx);
		ctx->tx_size = FT_MAX_CTRL_MSG + ft_tx_prefix_size(ctx);
		ctx->buf_size = ctx->rx_size + ctx->tx_size;
	}
	else
	{
		ft_set_tx_rx_sizes(ctx, &ctx->tx_size, &ctx->rx_size);
		ctx->tx_mr_size = 0;
		ctx->rx_mr_size = 0;
		ctx->buf_size = MAX(ctx->tx_size, FT_MAX_CTRL_MSG) * ctx->opts.window_size +
						MAX(ctx->rx_size, FT_MAX_CTRL_MSG) * ctx->opts.window_size;
	}

	if (ft_check_opts(ctx, FT_OPT_INIT_LARGE_BUFFER))
		ctx->buf_size = 1024 * 1024 * 1024 * 20ul;

	if (ctx->opts.options & FT_OPT_ALIGN && !(ctx->opts.options & FT_OPT_USE_DEVICE))
	{
		alignment = sysconf(_SC_PAGESIZE);
		if (alignment < 0)
			return -errno;
		ctx->buf_size += alignment;

		ret = posix_memalign((void **)&ctx->buf, (size_t)alignment,
							 ctx->buf_size);
		if (ret)
		{
			FT_PRINTERR("posix_memalign", ret);
			return ret;
		}
	}
	else
	{
		ret = ft_hmem_alloc(ctx->opts.iface, ctx->opts.device, (void **)&ctx->buf, ctx->buf_size);
		if (ret)
			return ret;
	}
	ret = ft_hmem_memset(ctx->opts.iface, ctx->opts.device, (void *)ctx->buf, 0, ctx->buf_size);
	if (ret)
		return ret;
	ctx->rx_buf = ctx->buf;

	if (ctx->opts.options & FT_OPT_ALLOC_MULT_MR)
		ctx->tx_buf = (char *)ctx->buf + MAX(ctx->rx_size, FT_MAX_CTRL_MSG);
	else
		ctx->tx_buf = (char *)ctx->buf + MAX(ctx->rx_size, FT_MAX_CTRL_MSG) * ctx->opts.window_size;

	ctx->remote_cq_data = ft_init_cq_data(ctx->fi);

	ctx->mr = &ctx->no_mr;
	if (!ft_mr_alloc_func && !ft_check_opts(ctx, FT_OPT_SKIP_REG_MR))
	{
		ret = ft_reg_mr(ctx, ctx->buf, ctx->buf_size, ft_info_to_mr_access(ctx->fi),
						FT_MR_KEY, &ctx->mr, &ctx->mr_desc);
		if (ret)
			return ret;
	}
	else
	{
		if (ft_mr_alloc_func)
		{
			assert(!ft_check_opts(ctx, FT_OPT_SKIP_REG_MR));
			ret = ft_mr_alloc_func();
			if (ret)
				return ret;
		}
	}

	ret = ft_alloc_ctx_array(ctx, &ctx->tx_ctx_arr, &ctx->tx_mr_bufs, ctx->tx_buf,
							 ctx->tx_mr_size, FT_TX_MR_KEY);
	if (ret)
		return -FI_ENOMEM;

	ret = ft_alloc_ctx_array(ctx, &ctx->rx_ctx_arr, &ctx->rx_mr_bufs, ctx->rx_buf,
							 ctx->rx_mr_size, FT_RX_MR_KEY);
	if (ret)
		return -FI_ENOMEM;

	return 0;
}

int ft_open_fabric_res(struct ConnectionContext *ctx)
{
	int ret;

	ret = fi_fabric(ctx->fi->fabric_attr, &ctx->fabric, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_fabric", ret);
		return ret;
	}

	ret = fi_eq_open(ctx->fabric, &ctx->eq_attr, &ctx->eq, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_eq_open", ret);
		return ret;
	}

	ret = fi_domain(ctx->fabric, ctx->fi, &ctx->domain, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_domain", ret);
		return ret;
	}

	if (ctx->opts.options & FT_OPT_DOMAIN_EQ)
	{
		ret = fi_domain_bind(ctx->domain, &ctx->eq->fid, 0);
		if (ret)
		{
			FT_PRINTERR("fi_domain_bind", ret);
			return ret;
		}
	}

	return 0;
}

int ft_alloc_ep_res(struct ConnectionContext *ctx, struct fi_info *fi)
{
	int ret;

	ret = ft_alloc_msgs(ctx);
	if (ret)
		return ret;

	if (ctx->cq_attr.format == FI_CQ_FORMAT_UNSPEC)
	{
		if (fi->caps & FI_TAGGED)
			ctx->cq_attr.format = FI_CQ_FORMAT_TAGGED;
		else
			ctx->cq_attr.format = FI_CQ_FORMAT_CONTEXT;
	}

	if (ctx->opts.options & FT_OPT_CQ_SHARED)
	{
		ft_cq_set_wait_attr(ctx);
		ctx->cq_attr.size = 0;

		if (ctx->opts.tx_cq_size)
			ctx->cq_attr.size += ctx->opts.tx_cq_size;
		else
			ctx->cq_attr.size += fi->tx_attr->size;

		if (ctx->opts.rx_cq_size)
			ctx->cq_attr.size += ctx->opts.rx_cq_size;
		else
			ctx->cq_attr.size += fi->rx_attr->size;

		ret = fi_cq_open(ctx->domain, &ctx->cq_attr, &ctx->txcq, &ctx->txcq);
		if (ret)
		{
			FT_PRINTERR("fi_cq_open", ret);
			return ret;
		}
		ctx->rxcq = ctx->txcq;
	}

	if (!(ctx->opts.options & FT_OPT_CQ_SHARED))
	{
		ft_cq_set_wait_attr(ctx);
		if (ctx->opts.tx_cq_size)
			ctx->cq_attr.size = ctx->opts.tx_cq_size;
		else
			ctx->cq_attr.size = fi->tx_attr->size;

		ret = fi_cq_open(ctx->domain, &ctx->cq_attr, &ctx->txcq, &ctx->txcq);
		if (ret)
		{
			FT_PRINTERR("fi_cq_open", ret);
			return ret;
		}
	}

	if (ctx->opts.options & FT_OPT_TX_CNTR)
	{
		ret = ft_cntr_open(ctx, &ctx->txcntr);
		if (ret)
		{
			FT_PRINTERR("fi_cntr_open", ret);
			return ret;
		}
	}

	if (!(ctx->opts.options & FT_OPT_CQ_SHARED))
	{
		ft_cq_set_wait_attr(ctx);
		if (ctx->opts.rx_cq_size)
			ctx->cq_attr.size = ctx->opts.rx_cq_size;
		else
			ctx->cq_attr.size = fi->rx_attr->size;

		ret = fi_cq_open(ctx->domain, &ctx->cq_attr, &ctx->rxcq, &ctx->rxcq);
		if (ret)
		{
			FT_PRINTERR("fi_cq_open", ret);
			return ret;
		}
	}

	if (ctx->opts.options & FT_OPT_RX_CNTR)
	{
		ret = ft_cntr_open(ctx, &ctx->rxcntr);
		if (ret)
		{
			FT_PRINTERR("fi_cntr_open", ret);
			return ret;
		}
	}

	if (fi->ep_attr->type == FI_EP_RDM || fi->ep_attr->type == FI_EP_DGRAM)
	{
		if (fi->domain_attr->av_type != FI_AV_UNSPEC)
			ctx->av_attr.type = fi->domain_attr->av_type;

		if (ctx->opts.av_name)
		{
			ctx->av_attr.name = ctx->opts.av_name;
		}
		ctx->av_attr.count = ctx->opts.av_size;
		ret = fi_av_open(ctx->domain, &ctx->av_attr, &ctx->av, NULL);
		if (ret)
		{
			FT_PRINTERR("fi_av_open", ret);
			return ret;
		}
	}
	return 0;
}

int ft_alloc_active_res(struct ConnectionContext *ctx, struct fi_info *fi)
{
	int ret;
	ret = ft_alloc_ep_res(ctx, fi);
	if (ret)
		return ret;

	ret = fi_endpoint(ctx->domain, fi, &ctx->ep, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_endpoint", ret);
		return ret;
	}

	return 0;
}

static int ft_init(struct ConnectionContext *ctx)
{
	ctx->tx_seq = 0;
	ctx->rx_seq = 0;
	ctx->tx_cq_cntr = 0;
	ctx->rx_cq_cntr = 0;

	//If using device memory for transfers, require OOB address
	//exchange because extra steps are involved when passing
	//device buffers into fi_av_insert
	if (ctx->opts.options & FT_OPT_ENABLE_HMEM)
		ctx->opts.options |= FT_OPT_OOB_ADDR_EXCH;

	return ft_hmem_init(ctx->opts.iface);
}

int ft_fd_nonblock(int fd)
{
	long flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return -errno;

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
		return -errno;

	return 0;
}

int ft_sock_setup(int sock)
{
	int ret, op;

	op = 1;
	ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
					 (void *)&op, sizeof(op));
	if (ret)
		return ret;

	ret = ft_fd_nonblock(sock);
	if (ret)
		return ret;

	return 0;
}

int ft_init_oob(struct ConnectionContext *ctx)
{
	struct addrinfo *ai = NULL;
	int ret;

	if (!(ctx->opts.options & FT_OPT_OOB_CTRL) || ctx->oob_sock != -1)
		return 0;

	if (!ctx->opts.oob_port)
		ctx->opts.oob_port = default_oob_port;

	if (!ctx->opts.dst_addr)
	{
		ret = ft_sock_listen(ctx, ctx->opts.src_addr, ctx->opts.oob_port);
		if (ret)
			return ret;

		ctx->oob_sock = accept(ctx->listen_sock, NULL, 0);
		if (ctx->oob_sock < 0)
		{
			perror("accept");
			ret = ctx->oob_sock;
			return ret;
		}

		close(ctx->listen_sock);
	}
	else
	{
		ret = getaddrinfo(ctx->opts.dst_addr, ctx->opts.oob_port, NULL, &ai);
		if (ret)
		{
			perror("getaddrinfo");
			return ret;
		}

		ctx->oob_sock = socket(ai->ai_family, SOCK_STREAM, 0);
		if (ctx->oob_sock < 0)
		{
			perror("socket");
			ret = ctx->oob_sock;
			goto free;
		}

		ret = connect(ctx->oob_sock, ai->ai_addr, ai->ai_addrlen);
		if (ret)
		{
			perror("connect");
			close(ctx->oob_sock);
			goto free;
		}
		sleep(1);
	}

	ret = ft_sock_setup(ctx->oob_sock);

free:
	if (ai)
		freeaddrinfo(ai);
	return ret;
}

int ft_accept_next_client(struct ConnectionContext *ctx)
{
	int ret;

	if (!ft_check_opts(ctx, FT_OPT_SKIP_MSG_ALLOC) && (ctx->fi->caps & (FI_MSG | FI_TAGGED)))
	{
		/* Initial receive will get remote address for unconnected EPs */
		ret = ft_post_rx(ctx, ctx->ep, MAX(ctx->rx_size, FT_MAX_CTRL_MSG), &ctx->rx_ctx);
		if (ret)
			return ret;
	}

	return ft_init_av(ctx);
}

int ft_getinfo(struct ConnectionContext *ctx, struct fi_info *hints, struct fi_info **info)
{
	char *node, *service;
	uint64_t flags = 0;
	int ret;

	ret = ft_read_addr_opts(&node, &service, hints, &flags, &ctx->opts);
	if (ret)
		return ret;

	if (!hints->ep_attr->type)
		hints->ep_attr->type = FI_EP_RDM;

	if (ctx->opts.options & FT_OPT_ENABLE_HMEM)
	{
		hints->caps |= FI_HMEM;
		hints->domain_attr->mr_mode |= FI_MR_HMEM;
	}

	ret = fi_getinfo(FT_FIVERSION, node, service, flags, hints, info);
	if (ret)
	{
		FT_PRINTERR("fi_getinfo", ret);
		return ret;
	}

	if (!ft_check_prefix_forced(*info, &ctx->opts))
	{
		FT_ERR("Provider disabled requested prefix mode.");
		return -FI_ENODATA;
	}

	return 0;
}

int ft_init_fabric_cm(struct ConnectionContext *ctx)
{
	int ret;
	if (!ctx->opts.dst_addr)
	{
		ret = ft_start_server(ctx);
		if (ret)
			return ret;
	}

	ret = ctx->opts.dst_addr ? ft_client_connect(ctx) : ft_server_connect(ctx);

	return ret;
}

int ft_start_server(struct ConnectionContext *ctx)
{
	int ret;

	ret = ft_init(ctx);
	if (ret)
		return ret;

	ret = ft_init_oob(ctx);
	if (ret)
		return ret;

	ret = ft_getinfo(ctx, ctx->hints, &ctx->fi_pep);
	if (ret)
		return ret;

	ret = fi_fabric(ctx->fi_pep->fabric_attr, &ctx->fabric, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_fabric", ret);
		return ret;
	}

	ret = fi_eq_open(ctx->fabric, &ctx->eq_attr, &ctx->eq, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_eq_open", ret);
		return ret;
	}

	ret = fi_passive_ep(ctx->fabric, ctx->fi_pep, &ctx->pep, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_passive_ep", ret);
		return ret;
	}

	ret = fi_pep_bind(ctx->pep, &ctx->eq->fid, 0);
	if (ret)
	{
		FT_PRINTERR("fi_pep_bind", ret);
		return ret;
	}

	ret = fi_listen(ctx->pep);
	if (ret)
	{
		FT_PRINTERR("fi_listen", ret);
		return ret;
	}

	return 0;
}

int ft_complete_connect(struct fid_ep *ep, struct fid_eq *eq)
{
	struct fi_eq_cm_entry entry;
	uint32_t event;
	ssize_t rd;
	int ret;

	rd = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
	if (rd != sizeof(entry))
	{
		FT_PROCESS_EQ_ERR(rd, eq, "fi_eq_sread", "accept");
		ret = (int)rd;
		return ret;
	}

	if (event != FI_CONNECTED || entry.fid != &ep->fid)
	{
		fprintf(stderr, "Unexpected CM event %d fid %p (ep %p)\n",
				event, entry.fid, ep);
		ret = -FI_EOTHER;
		return ret;
	}

	return 0;
}

int ft_retrieve_conn_req(struct fid_eq *eq, struct fi_info **fi)
{
	struct fi_eq_cm_entry entry;
	uint32_t event;
	ssize_t rd;
	int ret;

	rd = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
	if (rd != sizeof entry)
	{
		FT_PROCESS_EQ_ERR(rd, eq, "fi_eq_sread", "listen");
		return (int)rd;
	}

	*fi = entry.info;
	if (event != FI_CONNREQ)
	{
		fprintf(stderr, "Unexpected CM event %d\n", event);
		ret = -FI_EOTHER;
		return ret;
	}

	return 0;
}

int ft_accept_connection(struct fid_ep *ep, struct fid_eq *eq)
{
	int ret;

	ret = fi_accept(ep, NULL, 0);
	if (ret)
	{
		FT_PRINTERR("fi_accept", ret);
		return ret;
	}

	ret = ft_complete_connect(ep, eq);
	if (ret)
		return ret;

	return 0;
}

int ft_server_connect(struct ConnectionContext *ctx)
{
	int ret;

	ret = ft_retrieve_conn_req(ctx->eq, &ctx->fi);
	if (ret)
		goto err;

	ret = fi_domain(ctx->fabric, ctx->fi, &ctx->domain, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_domain", ret);
		goto err;
	}

	if (ctx->opts.options & FT_OPT_DOMAIN_EQ)
	{
		ret = fi_domain_bind(ctx->domain, &ctx->eq->fid, 0);
		if (ret)
		{
			FT_PRINTERR("fi_domain_bind", ret);
			return ret;
		}
	}

	ret = ft_alloc_active_res(ctx, ctx->fi);
	if (ret)
		goto err;

	ret = ft_enable_ep_recv(ctx);
	if (ret)
		goto err;

	ret = ft_accept_connection(ctx->ep, ctx->eq);
	if (ret)
		goto err;

	if (ft_check_opts(ctx, FT_OPT_FORK_CHILD))
		ft_fork_child();

	return 0;

err:
	if (ctx->fi)
		fi_reject(ctx->pep, ctx->fi->handle, NULL, 0);
	return ret;
}

int ft_connect_ep(struct fid_ep *ep,
				  struct fid_eq *eq, fi_addr_t *remote_addr)
{
	int ret;

	ret = fi_connect(ep, remote_addr, NULL, 0);
	if (ret)
	{
		FT_PRINTERR("fi_connect", ret);
		return ret;
	}

	ret = ft_complete_connect(ep, eq);
	if (ret)
		return ret;

	return 0;
}

int ft_client_connect(struct ConnectionContext *ctx)
{
	int ret;

	ret = ft_init(ctx);
	if (ret)
		return ret;

	ret = ft_init_oob(ctx);
	if (ret)
		return ret;

	ret = ft_getinfo(ctx, ctx->hints, &ctx->fi);
	if (ret)
		return ret;

	ret = ft_open_fabric_res(ctx);
	if (ret)
		return ret;

	ret = ft_alloc_active_res(ctx, ctx->fi);
	if (ret)
		return ret;

	ret = ft_enable_ep_recv(ctx);
	if (ret)
		return ret;

	ret = ft_connect_ep(ctx->ep, ctx->eq, ctx->fi->dest_addr);
	if (ret)
		return ret;

	if (ft_check_opts(ctx, FT_OPT_FORK_CHILD))
		ft_fork_child();

	return 0;
}

int ft_init_fabric(struct ConnectionContext *ctx)
{
	int ret;

	ret = ft_init(ctx);
	if (ret)
		return ret;

	ret = ft_init_oob(ctx);
	if (ret)
		return ret;

	ret = ft_getinfo(ctx, ctx->hints, &ctx->fi);
	if (ret)
		return ret;

	ret = ft_open_fabric_res(ctx);
	if (ret)
		return ret;

	ret = ft_alloc_active_res(ctx, ctx->fi);
	if (ret)
		return ret;

	ret = ft_enable_ep_recv(ctx);
	if (ret)
		return ret;

	ret = ft_init_av(ctx);
	if (ret)
		return ret;

	if (ft_check_opts(ctx, FT_OPT_FORK_CHILD))
		ft_fork_child();

	return 0;
}

int ft_get_cq_fd(struct ConnectionContext *ctx, struct fid_cq *cq, int *fd)
{
	int ret = FI_SUCCESS;

	if (cq && ctx->opts.comp_method == FT_COMP_WAIT_FD)
	{
		ret = fi_control(&cq->fid, FI_GETWAIT, fd);
		if (ret)
			FT_PRINTERR("fi_control(FI_GETWAIT)", ret);
	}

	return ret;
}

int ft_init_alias_ep(struct ConnectionContext *ctx, uint64_t flags)
{
	int ret;
	ret = fi_ep_alias(ctx->ep, &ctx->alias_ep, flags);
	if (ret)
	{
		FT_PRINTERR("fi_ep_alias", ret);
		return ret;
	}
	return 0;
}

int ft_enable_ep(struct ConnectionContext *ctx, struct fid_ep *ep, struct fid_eq *eq, struct fid_av *av,
				 struct fid_cq *txcq, struct fid_cq *rxcq,
				 struct fid_cntr *txcntr, struct fid_cntr *rxcntr)
{
	uint64_t flags;
	int ret;

	if ((ctx->fi->ep_attr->type == FI_EP_MSG || ctx->fi->caps & FI_MULTICAST ||
		 ctx->fi->caps & FI_COLLECTIVE) &&
		!(ctx->opts.options & FT_OPT_DOMAIN_EQ))
		FT_EP_BIND(ep, eq, 0);

	FT_EP_BIND(ep, av, 0);

	flags = FI_TRANSMIT;
	if (!(ctx->opts.options & FT_OPT_TX_CQ))
		flags |= FI_SELECTIVE_COMPLETION;
	if (ft_check_opts(ctx, FT_OPT_SELECTIVE_COMP))
		flags |= FI_SELECTIVE_COMPLETION;
	FT_EP_BIND(ep, txcq, flags);

	flags = FI_RECV;
	if (!(ctx->opts.options & FT_OPT_RX_CQ))
		flags |= FI_SELECTIVE_COMPLETION;
	FT_EP_BIND(ep, rxcq, flags);

	ret = ft_get_cq_fd(ctx, txcq, &ctx->tx_fd);
	if (ret)
		return ret;

	ret = ft_get_cq_fd(ctx, rxcq, &ctx->rx_fd);
	if (ret)
		return ret;

	/* TODO: use control structure to select counter bindings explicitly */
	if (ctx->opts.options & FT_OPT_TX_CQ)
		flags = 0;
	else
		flags = FI_SEND;
	if (ctx->hints->caps & (FI_WRITE | FI_READ))
		flags |= ctx->hints->caps & (FI_WRITE | FI_READ);
	else if (ctx->hints->caps & FI_RMA)
		flags |= FI_WRITE | FI_READ;
	FT_EP_BIND(ep, txcntr, flags);

	if (ctx->opts.options & FT_OPT_RX_CQ)
		flags = 0;
	else
		flags = FI_RECV;
	if (ctx->hints->caps & (FI_REMOTE_WRITE | FI_REMOTE_READ))
		flags |= ctx->hints->caps & (FI_REMOTE_WRITE | FI_REMOTE_READ);
	else if (ctx->hints->caps & FI_RMA)
		flags |= FI_REMOTE_WRITE | FI_REMOTE_READ;
	FT_EP_BIND(ep, rxcntr, flags);

	ret = fi_enable(ep);
	if (ret)
	{
		FT_PRINTERR("fi_enable", ret);
		return ret;
	}

	return 0;
}

int ft_enable_ep_recv(struct ConnectionContext *ctx)
{
	int ret;

	ret = ft_enable_ep(ctx, ctx->ep, ctx->eq, ctx->av, ctx->txcq, ctx->rxcq, ctx->txcntr, ctx->rxcntr);
	if (ret)
		return ret;

	if (!ft_check_opts(ctx, FT_OPT_SKIP_MSG_ALLOC) &&
		(ctx->fi->caps & (FI_MSG | FI_TAGGED)))
	{
		/* Initial receive will get remote address for unconnected EPs */
		ret = ft_post_rx(ctx, ctx->ep, MAX(ctx->rx_size, FT_MAX_CTRL_MSG), &ctx->rx_ctx);
		if (ret)
			return ret;
	}

	return 0;
}

int ft_join_mc(struct ConnectionContext *ctx)
{
	struct fi_eq_entry entry;
	uint32_t event;
	ssize_t rd;
	int ret;

	ret = fi_join(ctx->ep, ctx->fi->dest_addr, 0, &ctx->mc, ctx->ep->fid.context);
	if (ret)
	{
		FT_PRINTERR("fi_join", ret);
		return ret;
	}

	rd = fi_eq_sread(ctx->eq, &event, &entry, sizeof entry, -1, 0);
	if (rd != sizeof entry)
	{
		FT_PROCESS_EQ_ERR(rd, ctx->eq, "fi_eq_sread", "join");
		ret = (int)rd;
		return ret;
	}

	if (event != FI_JOIN_COMPLETE || entry.fid != &ctx->mc->fid)
	{
		fprintf(stderr, "Unexpected join event %d fid %p (mc %p)\n",
				event, entry.fid, ctx->ep);
		ret = -FI_EOTHER;
		return ret;
	}

	return 0;
}

int ft_av_insert(struct fid_av *av, void *addr, size_t count, fi_addr_t *fi_addr,
				 uint64_t flags, void *context)
{
	int ret;

	ret = fi_av_insert(av, addr, count, fi_addr, flags, context);
	if (ret < 0)
	{
		FT_PRINTERR("fi_av_insert", ret);
		return ret;
	}
	else if (ret != count)
	{
		FT_ERR("fi_av_insert: number of addresses inserted = %d;"
			   " number of addresses given = %zd\n",
			   ret, count);
		return -EXIT_FAILURE;
	}

	return 0;
}

int ft_init_av(struct ConnectionContext *ctx)
{
	return ft_init_av_dst_addr(ctx, ctx->av, ctx->ep, &ctx->remote_fi_addr);
}

int ft_exchange_addresses_oob(struct ConnectionContext *ctx, struct fid_av *av_ptr, struct fid_ep *ep_ptr,
							  fi_addr_t *remote_addr)
{
	char buf[FT_MAX_CTRL_MSG];
	int ret;
	size_t addrlen = FT_MAX_CTRL_MSG;

	ret = fi_getname(&ep_ptr->fid, buf, &addrlen);
	if (ret)
	{
		FT_PRINTERR("fi_getname", ret);
		return ret;
	}

	ret = ft_sock_send(ctx, ctx->oob_sock, buf, FT_MAX_CTRL_MSG);
	if (ret)
		return ret;

	ret = ft_sock_recv(ctx, ctx->oob_sock, buf, FT_MAX_CTRL_MSG);
	if (ret)
		return ret;

	ret = ft_av_insert(av_ptr, buf, 1, remote_addr, 0, NULL);
	if (ret)
		return ret;

	return 0;
}

/* TODO: retry send for unreliable endpoints */
int ft_init_av_dst_addr(struct ConnectionContext *ctx, struct fid_av *av_ptr, struct fid_ep *ep_ptr,
						fi_addr_t *remote_addr)
{
	size_t addrlen;
	int ret;

	if (ctx->opts.options & FT_OPT_OOB_ADDR_EXCH)
	{
		ret = ft_exchange_addresses_oob(ctx, av_ptr, ep_ptr, remote_addr);
		if (ret)
			return ret;
		else
			goto set_rx_seq_close;
	}

	if (ctx->opts.dst_addr)
	{
		ret = ft_av_insert(av_ptr, ctx->fi->dest_addr, 1, remote_addr, 0, NULL);
		if (ret)
			return ret;

		addrlen = FT_MAX_CTRL_MSG;
		ret = fi_getname(&ep_ptr->fid, (char *)ctx->tx_buf + ft_tx_prefix_size(ctx),
						 &addrlen);
		if (ret)
		{
			FT_PRINTERR("fi_getname", ret);
			return ret;
		}

		ret = (int)ft_tx(ctx, ctx->ep, *remote_addr, addrlen, &ctx->tx_ctx);
		if (ret)
			return ret;

		ret = ft_rx(ctx, ctx->ep, 1);
		if (ret)
			return ret;
	}
	else
	{
		ret = ft_get_rx_comp(ctx, ctx->rx_seq);
		if (ret)
			return ret;

		/* Test passing NULL fi_addr on one of the sides (server) if
		 * AV type is FI_AV_TABLE */
		ret = ft_av_insert(av_ptr, (char *)ctx->rx_buf + ft_rx_prefix_size(ctx),
						   1, ((ctx->fi->domain_attr->av_type == FI_AV_TABLE) ? NULL : remote_addr), 0, NULL);
		if (ret)
			return ret;

		ret = ft_post_rx(ctx, ctx->ep, ctx->rx_size, &ctx->rx_ctx);
		if (ret)
			return ret;

		if (ctx->fi->domain_attr->av_type == FI_AV_TABLE)
			*remote_addr = 0;

		ret = (int)ft_tx(ctx, ctx->ep, *remote_addr, 1, &ctx->tx_ctx);
		if (ret)
			return ret;
	}

set_rx_seq_close:
	/*
	* For a test which does not have MSG or TAGGED
	* capabilities, but has RMA/Atomics and uses the OOB sync.
	* If no recv is going to be posted,
	* then the rx_seq needs to be incremented to wait on the first RMA/Atomic
	* completion.
	*/
	if (!(ctx->fi->caps & FI_MSG) && !(ctx->fi->caps & FI_TAGGED) && ctx->opts.oob_port)
		ctx->rx_seq++;

	return 0;
}

/* TODO: retry send for unreliable endpoints */
int ft_init_av_addr(struct ConnectionContext *ctx, struct fid_av *av_ptr, struct fid_ep *ep_ptr,
					fi_addr_t *remote_addr)
{
	size_t addrlen;
	int ret;

	if (ctx->opts.options & FT_OPT_OOB_ADDR_EXCH)
		return ft_exchange_addresses_oob(ctx, av_ptr, ep_ptr, remote_addr);

	if (ctx->opts.dst_addr)
	{
		addrlen = FT_MAX_CTRL_MSG;
		ret = fi_getname(&ep_ptr->fid, (char *)ctx->tx_buf + ft_tx_prefix_size(ctx),
						 &addrlen);
		if (ret)
		{
			FT_PRINTERR("fi_getname", ret);
			return ret;
		}

		ret = (int)ft_tx(ctx, ctx->ep, ctx->remote_fi_addr, addrlen, &ctx->tx_ctx);
		if (ret)
			return ret;

		ret = (int)ft_rx(ctx, ctx->ep, FT_MAX_CTRL_MSG);
		if (ret)
			return ret;

		ret = ft_av_insert(av_ptr, (char *)ctx->rx_buf + ft_rx_prefix_size(ctx),
						   1, remote_addr, 0, NULL);
		if (ret)
			return ret;
	}
	else
	{
		ret = (int)ft_rx(ctx, ctx->ep, FT_MAX_CTRL_MSG);
		if (ret)
			return ret;

		ret = ft_av_insert(av_ptr, (char *)ctx->rx_buf + ft_rx_prefix_size(ctx),
						   1, remote_addr, 0, NULL);
		if (ret)
			return ret;

		addrlen = FT_MAX_CTRL_MSG;
		ret = fi_getname(&ep_ptr->fid,
						 (char *)ctx->tx_buf + ft_tx_prefix_size(ctx),
						 &addrlen);
		if (ret)
		{
			FT_PRINTERR("fi_getname", ret);
			return ret;
		}

		ret = (int)ft_tx(ctx, ctx->ep, ctx->remote_fi_addr, addrlen, &ctx->tx_ctx);
		if (ret)
			return ret;
	}

	return 0;
}

int ft_exchange_raw_keys(struct ConnectionContext *ctx, struct fi_rma_iov *peer_iov)
{
	struct fi_rma_iov *rma_iov;
	size_t key_size;
	size_t len;
	uint64_t addr;
	int ret;

	/* Get key size */
	key_size = 0;
	ret = fi_mr_raw_attr(ctx->mr, &addr, NULL, &key_size, 0);
	if (ret != -FI_ETOOSMALL)
	{
		return ret;
	}

	len = sizeof(*rma_iov) + key_size - sizeof(rma_iov->key);
	/* TODO: make sure this fits in tx_buf and rx_buf */

	if (ctx->opts.dst_addr)
	{
		rma_iov = (struct fi_rma_iov *)(ctx->tx_buf + ft_tx_prefix_size(ctx));
		if ((ctx->fi->domain_attr->mr_mode == FI_MR_BASIC) ||
			(ctx->fi->domain_attr->mr_mode & FI_MR_VIRT_ADDR))
		{
			rma_iov->addr = (uintptr_t)ctx->rx_buf + ft_rx_prefix_size(ctx);
		}
		else
		{
			rma_iov->addr = 0;
		}

		/* Get raw attributes */
		ret = fi_mr_raw_attr(ctx->mr, &addr, (uint8_t *)&rma_iov->key,
							 &key_size, 0);
		if (ret)
			return ret;

		ret = ft_tx(ctx, ctx->ep, ctx->remote_fi_addr, len, &ctx->tx_ctx);
		if (ret)
			return ret;

		ret = ft_get_rx_comp(ctx, ctx->rx_seq);
		if (ret)
			return ret;

		rma_iov = (struct fi_rma_iov *)(ctx->rx_buf + ft_rx_prefix_size(ctx));
		peer_iov->addr = rma_iov->addr;
		peer_iov->len = rma_iov->len;
		/* Map remote mr raw locally */
		ret = fi_mr_map_raw(ctx->domain, rma_iov->addr,
							(uint8_t *)&rma_iov->key, key_size,
							&peer_iov->key, 0);
		if (ret)
			return ret;

		ret = ft_post_rx(ctx, ctx->ep, ctx->rx_size, &ctx->rx_ctx);
	}
	else
	{
		ret = ft_get_rx_comp(ctx, ctx->rx_seq);
		if (ret)
			return ret;

		rma_iov = (struct fi_rma_iov *)(ctx->rx_buf + ft_rx_prefix_size(ctx));
		peer_iov->addr = rma_iov->addr;
		peer_iov->len = rma_iov->len;
		/* Map remote mr raw locally */
		ret = fi_mr_map_raw(ctx->domain, rma_iov->addr,
							(uint8_t *)&rma_iov->key, key_size,
							&peer_iov->key, 0);
		if (ret)
			return ret;

		ret = ft_post_rx(ctx, ctx->ep, ctx->rx_size, &ctx->rx_ctx);
		if (ret)
			return ret;

		rma_iov = (struct fi_rma_iov *)(ctx->tx_buf + ft_tx_prefix_size(ctx));
		if ((ctx->fi->domain_attr->mr_mode == FI_MR_BASIC) ||
			(ctx->fi->domain_attr->mr_mode & FI_MR_VIRT_ADDR))
		{
			rma_iov->addr = (uintptr_t)ctx->rx_buf + ft_rx_prefix_size(ctx);
		}
		else
		{
			rma_iov->addr = 0;
		}

		/* Get raw attributes */
		ret = fi_mr_raw_attr(ctx->mr, &addr, (uint8_t *)&rma_iov->key,
							 &key_size, 0);
		if (ret)
			return ret;

		ret = ft_tx(ctx, ctx->ep, ctx->remote_fi_addr, len, &ctx->tx_ctx);
	}

	return ret;
}

int ft_exchange_keys(struct ConnectionContext *ctx, struct fi_rma_iov *peer_iov)
{
	struct fi_rma_iov *rma_iov;
	int ret;

	if (ctx->fi->domain_attr->mr_mode & FI_MR_RAW)
		return ft_exchange_raw_keys(ctx, peer_iov);

	if (ctx->opts.dst_addr)
	{
		rma_iov = (struct fi_rma_iov *)(ctx->tx_buf + ft_tx_prefix_size(ctx));
		if ((ctx->fi->domain_attr->mr_mode == FI_MR_BASIC) ||
			(ctx->fi->domain_attr->mr_mode & FI_MR_VIRT_ADDR))
		{
			rma_iov->addr = (uintptr_t)ctx->rx_buf + ft_rx_prefix_size(ctx);
		}
		else
		{
			rma_iov->addr = 0;
		}
		rma_iov->key = fi_mr_key(ctx->mr);
		ret = ft_tx(ctx, ctx->ep, ctx->remote_fi_addr, sizeof *rma_iov, &ctx->tx_ctx);
		if (ret)
			return ret;

		ret = ft_get_rx_comp(ctx, ctx->rx_seq);
		if (ret)
			return ret;

		rma_iov = (struct fi_rma_iov *)(ctx->rx_buf + ft_rx_prefix_size(ctx));
		*peer_iov = *rma_iov;
		ret = ft_post_rx(ctx, ctx->ep, ctx->rx_size, &ctx->rx_ctx);
	}
	else
	{
		ret = ft_get_rx_comp(ctx, ctx->rx_seq);
		if (ret)
			return ret;

		rma_iov = (struct fi_rma_iov *)(ctx->rx_buf + ft_rx_prefix_size(ctx));
		*peer_iov = *rma_iov;
		ret = ft_post_rx(ctx, ctx->ep, ctx->rx_size, &ctx->rx_ctx);
		if (ret)
			return ret;

		rma_iov = (struct fi_rma_iov *)(ctx->tx_buf + ft_tx_prefix_size(ctx));
		if ((ctx->fi->domain_attr->mr_mode == FI_MR_BASIC) ||
			(ctx->fi->domain_attr->mr_mode & FI_MR_VIRT_ADDR))
		{
			rma_iov->addr = (uintptr_t)ctx->rx_buf + ft_rx_prefix_size(ctx);
		}
		else
		{
			rma_iov->addr = 0;
		}
		rma_iov->key = fi_mr_key(ctx->mr);
		ret = ft_tx(ctx, ctx->ep, ctx->remote_fi_addr, sizeof *rma_iov, &ctx->tx_ctx);
	}

	return ret;
}

static void ft_cleanup_mr_array(struct ConnectionContext *ctx, struct ft_context *ctx_arr, char **mr_bufs)
{
	int i, ret;

	if (!mr_bufs)
		return;

	for (i = 0; i < ctx->opts.window_size; i++)
	{
		FT_CLOSE_FID(ctx_arr[i].mr);
		ret = ft_hmem_free(ctx->opts.iface, mr_bufs[i]);
		if (ret)
			FT_PRINTERR("ft_hmem_free", ret);
	}
}

static void ft_close_fids(struct ConnectionContext *ctx)
{
	FT_CLOSE_FID(ctx->mc);
	FT_CLOSE_FID(ctx->alias_ep);
	FT_CLOSE_FID(ctx->ep);
	FT_CLOSE_FID(ctx->pep);
	if (ctx->opts.options & FT_OPT_CQ_SHARED)
	{
		FT_CLOSE_FID(ctx->txcq);
	}
	else
	{
		FT_CLOSE_FID(ctx->rxcq);
		FT_CLOSE_FID(ctx->txcq);
	}
	FT_CLOSE_FID(ctx->rxcntr);
	FT_CLOSE_FID(ctx->txcntr);
	FT_CLOSE_FID(ctx->pollset);
	if (ctx->mr != &ctx->no_mr)
		FT_CLOSE_FID(ctx->mr);
	FT_CLOSE_FID(ctx->av);
	FT_CLOSE_FID(ctx->domain);
	FT_CLOSE_FID(ctx->eq);
	FT_CLOSE_FID(ctx->waitset);
	FT_CLOSE_FID(ctx->fabric);
}

void ft_free_res(struct ConnectionContext *ctx)
{
	int ret;

	ft_cleanup_mr_array(ctx, ctx->tx_ctx_arr, ctx->tx_mr_bufs);
	ft_cleanup_mr_array(ctx, ctx->rx_ctx_arr, ctx->rx_mr_bufs);

	free(ctx->tx_ctx_arr);
	free(ctx->rx_ctx_arr);
	ctx->tx_ctx_arr = NULL;
	ctx->rx_ctx_arr = NULL;

	ft_close_fids(ctx);

	if (ctx->buf)
	{
		ret = ft_hmem_free(ctx->opts.iface, ctx->buf);
		if (ret)
			FT_PRINTERR("ft_hmem_free", ret);
		ctx->buf = ctx->rx_buf = ctx->tx_buf = NULL;
		ctx->buf_size = ctx->rx_size = ctx->tx_size = ctx->tx_mr_size = ctx->rx_mr_size = 0;
	}
	if (ctx->fi_pep)
	{
		fi_freeinfo(ctx->fi_pep);
		ctx->fi_pep = NULL;
	}
	if (ctx->fi)
	{
		fi_freeinfo(ctx->fi);
		ctx->fi = NULL;
	}
	if (ctx->hints)
	{
		fi_freeinfo(ctx->hints);
		ctx->hints = NULL;
	}

	ret = ft_hmem_cleanup(ctx->opts.iface);
	if (ret)
		FT_PRINTERR("ft_hmem_cleanup", ret);
}

static int dupaddr(void **dst_addr, size_t *dst_addrlen,
				   void *src_addr, size_t src_addrlen)
{
	*dst_addr = malloc(src_addrlen);
	if (!*dst_addr)
	{
		FT_ERR("address allocation failed");
		return EAI_MEMORY;
	}
	*dst_addrlen = src_addrlen;
	memcpy(*dst_addr, src_addr, src_addrlen);
	return 0;
}

static int getaddr(char *node, char *service,
				   struct fi_info *hints, uint64_t flags)
{
	int ret;
	struct fi_info *fi;

	if (!node && !service)
	{
		if (flags & FI_SOURCE)
		{
			hints->src_addr = NULL;
			hints->src_addrlen = 0;
		}
		else
		{
			hints->dest_addr = NULL;
			hints->dest_addrlen = 0;
		}
		return 0;
	}

	ret = fi_getinfo(FT_FIVERSION, node, service, flags, hints, &fi);
	if (ret)
	{
		FT_PRINTERR("fi_getinfo", ret);
		return ret;
	}
	hints->addr_format = fi->addr_format;

	if (flags & FI_SOURCE)
	{
		ret = dupaddr(&hints->src_addr, &hints->src_addrlen,
					  fi->src_addr, fi->src_addrlen);
	}
	else
	{
		ret = dupaddr(&hints->dest_addr, &hints->dest_addrlen,
					  fi->dest_addr, fi->dest_addrlen);
	}

	fi_freeinfo(fi);
	return ret;
}

int ft_getsrcaddr(char *node, char *service, struct fi_info *hints)
{
	return getaddr(node, service, hints, FI_SOURCE);
}

int ft_read_addr_opts(char **node, char **service, struct fi_info *hints,
					  uint64_t *flags, struct ft_opts *opts)
{
	int ret;

	if (opts->dst_addr && (opts->src_addr || !opts->oob_port))
	{
		if (!opts->dst_port)
			opts->dst_port = default_port;

		ret = ft_getsrcaddr(opts->src_addr, opts->src_port, hints);
		if (ret)
			return ret;
		*node = opts->dst_addr;
		*service = opts->dst_port;
	}
	else
	{
		if (!opts->src_port)
			opts->src_port = default_port;

		*node = opts->src_addr;
		*service = opts->src_port;
		*flags = FI_SOURCE;
	}

	return 0;
}

char *size_str(char str[FT_STR_LEN], long long size)
{
	long long base, fraction = 0;
	char mag;

	memset(str, '\0', FT_STR_LEN);

	if (size >= (1 << 30))
	{
		base = 1 << 30;
		mag = 'g';
	}
	else if (size >= (1 << 20))
	{
		base = 1 << 20;
		mag = 'm';
	}
	else if (size >= (1 << 10))
	{
		base = 1 << 10;
		mag = 'k';
	}
	else
	{
		base = 1;
		mag = '\0';
	}

	if (size / base < 10)
		fraction = (size % base) * 10 / base;

	if (fraction)
		snprintf(str, FT_STR_LEN, "%lld.%lld%c", size / base, fraction, mag);
	else
		snprintf(str, FT_STR_LEN, "%lld%c", size / base, mag);

	return str;
}

char *cnt_str(char str[FT_STR_LEN], long long cnt)
{
	if (cnt >= 1000000000)
		snprintf(str, FT_STR_LEN, "%lldb", cnt / 1000000000);
	else if (cnt >= 1000000)
		snprintf(str, FT_STR_LEN, "%lldm", cnt / 1000000);
	else if (cnt >= 1000)
		snprintf(str, FT_STR_LEN, "%lldk", cnt / 1000);
	else
		snprintf(str, FT_STR_LEN, "%lld", cnt);

	return str;
}

int size_to_count(struct ConnectionContext *ctx, int size)
{
	if (size >= (1 << 20))
		return (ctx->opts.options & FT_OPT_BW) ? 200 : 100;
	else if (size >= (1 << 16))
		return (ctx->opts.options & FT_OPT_BW) ? 2000 : 1000;
	else
		return (ctx->opts.options & FT_OPT_BW) ? 20000 : 10000;
}

static const size_t datatype_size_table[] = {
	[FI_INT8] = sizeof(int8_t),
	[FI_UINT8] = sizeof(uint8_t),
	[FI_INT16] = sizeof(int16_t),
	[FI_UINT16] = sizeof(uint16_t),
	[FI_INT32] = sizeof(int32_t),
	[FI_UINT32] = sizeof(uint32_t),
	[FI_INT64] = sizeof(int64_t),
	[FI_UINT64] = sizeof(uint64_t),
	[FI_FLOAT] = sizeof(float),
	[FI_DOUBLE] = sizeof(double),
	[FI_LONG_DOUBLE] = sizeof(long double),
};

size_t datatype_to_size(enum fi_datatype datatype)
{
	if (datatype >= FI_DATATYPE_LAST)
		return 0;

	return datatype_size_table[datatype];
}

void init_test(struct ConnectionContext *ctx, struct ft_opts *opts, char *test_name, size_t test_name_len)
{
	char sstr[FT_STR_LEN];

	size_str(sstr, opts->transfer_size);
	if (!strcmp(test_name, "custom"))
		snprintf(test_name, test_name_len, "%s_lat", sstr);
	if (!(opts->options & FT_OPT_ITER))
		opts->iterations = size_to_count(ctx, opts->transfer_size);
}

static void ft_force_progress(struct ConnectionContext *ctx)
{
	if (ctx->txcq)
		fi_cq_read(ctx->txcq, NULL, 0);
	if (ctx->rxcq)
		fi_cq_read(ctx->rxcq, NULL, 0);
}

static int ft_progress(struct fid_cq *cq, uint64_t total, uint64_t *cq_cntr)
{
	struct fi_cq_err_entry comp;
	int ret;

	ret = fi_cq_read(cq, &comp, 1);
	if (ret > 0)
		(*cq_cntr)++;

	if (ret >= 0 || ret == -FI_EAGAIN)
		return 0;

	if (ret == -FI_EAVAIL)
	{
		ret = ft_cq_readerr(cq);
		(*cq_cntr)++;
	}
	else
	{
		FT_PRINTERR("fi_cq_read/sread", ret);
	}
	return ret;
}

#define FT_POST(post_fn, progress_fn, cq, seq, cq_cntr, op_str, ...) \
	do                                                               \
	{                                                                \
		int timeout_save;                                            \
		int ret, rc;                                                 \
                                                                     \
		while (1)                                                    \
		{                                                            \
			ret = post_fn(__VA_ARGS__);                              \
			if (!ret)                                                \
				break;                                               \
                                                                     \
			if (ret != -FI_EAGAIN)                                   \
			{                                                        \
				FT_PRINTERR(op_str, ret);                            \
				return ret;                                          \
			}                                                        \
                                                                     \
			timeout_save = ctx->timeout;                             \
			ctx->timeout = 0;                                        \
			rc = progress_fn(cq, seq, cq_cntr);                      \
			if (rc && rc != -FI_EAGAIN)                              \
			{                                                        \
				FT_ERR("Failed to get " op_str " completion");       \
				return rc;                                           \
			}                                                        \
			ctx->timeout = timeout_save;                             \
		}                                                            \
		seq++;                                                       \
	} while (0)

ssize_t ft_post_tx_buf(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr, size_t size,
					   uint64_t data, void *ctxptr,
					   void *op_buf, void *op_mr_desc, uint64_t op_tag)
{
	size += ft_tx_prefix_size(ctx);
	if (ctx->hints->caps & FI_TAGGED)
	{
		op_tag = op_tag ? op_tag : ctx->tx_seq;
		if (data != NO_CQ_DATA)
		{
			FT_POST(fi_tsenddata, ft_progress, ctx->txcq, ctx->tx_seq,
					&ctx->tx_cq_cntr, "transmit", ep, op_buf, size,
					op_mr_desc, data, fi_addr, op_tag, ctxptr);
		}
		else
		{
			FT_POST(fi_tsend, ft_progress, ctx->txcq, ctx->tx_seq,
					&ctx->tx_cq_cntr, "transmit", ep, op_buf, size,
					op_mr_desc, fi_addr, op_tag, ctxptr);
		}
	}
	else
	{
		if (data != NO_CQ_DATA)
		{
			FT_POST(fi_senddata, ft_progress, ctx->txcq, ctx->tx_seq,
					&ctx->tx_cq_cntr, "transmit", ep, op_buf, size,
					op_mr_desc, data, fi_addr, ctxptr);
		}
		else
		{
			FT_POST(fi_send, ft_progress, ctx->txcq, ctx->tx_seq,
					&ctx->tx_cq_cntr, "transmit", ep, op_buf, size,
					op_mr_desc, fi_addr, ctxptr);
		}
	}
	return 0;
}

ssize_t ft_post_tx(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr, size_t size,
				   uint64_t data, void *ctxptr)
{
	if (ft_check_opts(ctx, FT_OPT_SELECTIVE_COMP))
	{
		size += ft_tx_prefix_size(ctx);
		return ft_sendmsg(ctx, ep, fi_addr, size, ctxptr, FI_COMPLETION);
	}
	else
	{
		return ft_post_tx_buf(ctx, ep, fi_addr, size, data,
							  ctxptr, ctx->tx_buf, ctx->mr_desc, ctx->ft_tag);
	}
}

ssize_t ft_tx(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr, size_t size, void *ctxptr)
{
	ssize_t ret;

	if (ft_check_opts(ctx, FT_OPT_VERIFY_DATA | FT_OPT_ACTIVE))
	{
		ret = ft_fill_buf(ctx, (char *)ctx->tx_buf + ft_tx_prefix_size(ctx), size);
		if (ret)
			return ret;
	}

	ret = ft_post_tx(ctx, ep, fi_addr, size, NO_CQ_DATA, ctxptr);
	if (ret)
		return ret;

	ret = ft_get_tx_comp(ctx, ctx->tx_seq);
	return ret;
}

ssize_t ft_post_inject(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr, size_t size)
{
	if (ctx->hints->caps & FI_TAGGED)
	{
		FT_POST(fi_tinject, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"inject", ep, ctx->tx_buf, size + ft_tx_prefix_size(ctx),
				fi_addr, ctx->tx_seq);
	}
	else
	{
		FT_POST(fi_inject, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"inject", ep, ctx->tx_buf, size + ft_tx_prefix_size(ctx),
				fi_addr);
	}

	ctx->tx_cq_cntr++;
	return 0;
}

ssize_t ft_inject(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr, size_t size)
{
	ssize_t ret;

	if (ft_check_opts(ctx, FT_OPT_VERIFY_DATA | FT_OPT_ACTIVE))
	{
		ret = ft_fill_buf(ctx, (char *)ctx->tx_buf + ft_tx_prefix_size(ctx), size);
		if (ret)
			return ret;
	}

	ret = ft_post_inject(ctx, ep, fi_addr, size);
	if (ret)
		return ret;

	return ret;
}

ssize_t ft_post_rma(struct ConnectionContext *ctx, enum ft_rma_opcodes op, struct fid_ep *ep, size_t size,
					struct fi_rma_iov *remote, void *context)
{
	switch (op)
	{
	case FT_RMA_WRITE:
		FT_POST(fi_write, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"fi_write", ep, ctx->tx_buf, ctx->opts.transfer_size, ctx->mr_desc,
				ctx->remote_fi_addr, remote->addr, remote->key, context);
		break;
	case FT_RMA_WRITEDATA:
		FT_POST(fi_writedata, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"fi_writedata", ep, ctx->tx_buf, ctx->opts.transfer_size, ctx->mr_desc,
				ctx->remote_cq_data, ctx->remote_fi_addr, remote->addr,
				remote->key, context);
		break;
	case FT_RMA_READ:
		FT_POST(fi_read, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"fi_read", ep, ctx->rx_buf, ctx->opts.transfer_size, ctx->mr_desc,
				ctx->remote_fi_addr, remote->addr, remote->key, context);
		break;
	default:
		FT_ERR("Unknown RMA op type\n");
		return EXIT_FAILURE;
	}

	return 0;
}

ssize_t ft_rma(struct ConnectionContext *ctx, enum ft_rma_opcodes op, struct fid_ep *ep, size_t size,
			   struct fi_rma_iov *remote, void *context)
{
	int ret;

	ret = ft_post_rma(ctx, op, ep, size, remote, context);
	if (ret)
		return ret;

	if (op == FT_RMA_WRITEDATA)
	{
		if (ctx->fi->rx_attr->mode & FI_RX_CQ_DATA)
		{
			ret = ft_rx(ctx, ep, 0);
		}
		else
		{
			ret = ft_get_rx_comp(ctx, ctx->rx_seq);
			/* Just increment the seq # instead of posting recv so
			 * that we wait for remote write completion on the next
			 * iteration. */
			ctx->rx_seq++;
		}
		if (ret)
			return ret;
	}

	ret = ft_get_tx_comp(ctx, ctx->tx_seq);
	if (ret)
		return ret;

	return 0;
}

ssize_t ft_post_rma_inject(struct ConnectionContext *ctx, enum ft_rma_opcodes op, struct fid_ep *ep, size_t size,
						   struct fi_rma_iov *remote)
{
	switch (op)
	{
	case FT_RMA_WRITE:
		FT_POST(fi_inject_write, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"fi_inject_write", ep, ctx->tx_buf, ctx->opts.transfer_size,
				ctx->remote_fi_addr, remote->addr, remote->key + 1);
		break;
	case FT_RMA_WRITEDATA:
		FT_POST(fi_inject_writedata, ft_progress, ctx->txcq, ctx->tx_seq,
				&ctx->tx_cq_cntr, "fi_inject_writedata", ep, ctx->tx_buf,
				ctx->opts.transfer_size, ctx->remote_cq_data, ctx->remote_fi_addr,
				remote->addr, remote->key);
		break;
	default:
		FT_ERR("Unknown RMA inject op type\n");
		return EXIT_FAILURE;
	}

	ctx->tx_cq_cntr++;
	return 0;
}

ssize_t ft_post_atomic(struct ConnectionContext *ctx, enum ft_atomic_opcodes opcode, struct fid_ep *ep,
					   void *compare, void *compare_desc, void *result,
					   void *result_desc, struct fi_rma_iov *remote,
					   enum fi_datatype datatype, enum fi_op atomic_op,
					   void *context)
{
	size_t size, count;

	size = datatype_to_size(datatype);
	if (!size)
	{
		FT_ERR("Unknown datatype\n");
		return EXIT_FAILURE;
	}
	count = ctx->opts.transfer_size / size;

	switch (opcode)
	{
	case FT_ATOMIC_BASE:
		FT_POST(fi_atomic, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"fi_atomic", ep, ctx->buf, count, ctx->mr_desc, ctx->remote_fi_addr,
				remote->addr, remote->key, datatype, atomic_op, context);
		break;
	case FT_ATOMIC_FETCH:
		FT_POST(fi_fetch_atomic, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"fi_fetch_atomic", ep, ctx->buf, count, ctx->mr_desc, result,
				result_desc, ctx->remote_fi_addr, remote->addr, remote->key,
				datatype, atomic_op, context);
		break;
	case FT_ATOMIC_COMPARE:
		FT_POST(fi_compare_atomic, ft_progress, ctx->txcq, ctx->tx_seq,
				&ctx->tx_cq_cntr, "fi_compare_atomic", ep, ctx->buf, count,
				ctx->mr_desc, compare, compare_desc, result, result_desc,
				ctx->remote_fi_addr, remote->addr, remote->key, datatype,
				atomic_op, context);
		break;
	default:
		FT_ERR("Unknown atomic opcode\n");
		return EXIT_FAILURE;
	}

	return 0;
}

static int check_atomic_attr(struct ConnectionContext *ctx, enum fi_op op, enum fi_datatype datatype,
							 uint64_t flags)
{
	struct fi_atomic_attr attr;
	int ret;

	ret = fi_query_atomic(ctx->domain, datatype, op, &attr, flags);
	if (ret)
	{
		FT_PRINTERR("fi_query_atomic", ret);
		return ret;
	}

	if (attr.size != datatype_to_size(datatype))
	{
		fprintf(stderr, "Provider atomic size mismatch\n");
		return -FI_ENOSYS;
	}

	return 0;
}

int check_base_atomic_op(struct ConnectionContext *ctx, struct fid_ep *endpoint, enum fi_op op,
						 enum fi_datatype datatype, size_t *count)
{
	int ret;

	ret = fi_atomicvalid(endpoint, datatype, op, count);
	if (ret)
		return ret;

	return check_atomic_attr(ctx, op, datatype, 0);
}

int check_fetch_atomic_op(struct ConnectionContext *ctx, struct fid_ep *endpoint, enum fi_op op,
						  enum fi_datatype datatype, size_t *count)
{
	int ret;

	ret = fi_fetch_atomicvalid(endpoint, datatype, op, count);
	if (ret)
		return ret;

	return check_atomic_attr(ctx, op, datatype, FI_FETCH_ATOMIC);
}

int check_compare_atomic_op(struct ConnectionContext *ctx, struct fid_ep *endpoint, enum fi_op op,
							enum fi_datatype datatype, size_t *count)
{
	int ret;

	ret = fi_compare_atomicvalid(endpoint, datatype, op, count);
	if (ret)
		return ret;

	return check_atomic_attr(ctx, op, datatype, FI_COMPARE_ATOMIC);
}

ssize_t ft_post_rx_buf(struct ConnectionContext *ctx, struct fid_ep *ep, size_t size, void *ctxptr,
					   void *op_buf, void *op_mr_desc, uint64_t op_tag)
{
	size = MAX(size, FT_MAX_CTRL_MSG) + ft_rx_prefix_size(ctx);
	if (ctx->hints->caps & FI_TAGGED)
	{
		op_tag = op_tag ? op_tag : ctx->rx_seq;
		FT_POST(fi_trecv, ft_progress, ctx->rxcq, ctx->rx_seq, &ctx->rx_cq_cntr,
				"receive", ep, op_buf, size, op_mr_desc,
				ctx->remote_fi_addr, op_tag, 0, ctx);
	}
	else
	{
		FT_POST(fi_recv, ft_progress, ctx->rxcq, ctx->rx_seq, &ctx->rx_cq_cntr,
				"receive", ep, op_buf, size, op_mr_desc, 0, ctx);
	}
	return 0;
}

ssize_t ft_post_rx(struct ConnectionContext *ctx, struct fid_ep *ep, size_t size, void *ctxptr)
{
	return ft_post_rx_buf(ctx, ep, size, ctxptr, ctx->rx_buf, ctx->mr_desc, ctx->ft_tag);
}

ssize_t ft_rx(struct ConnectionContext *ctx, struct fid_ep *ep, size_t size)
{
	ssize_t ret;

	ret = ft_get_rx_comp(ctx, ctx->rx_seq);
	if (ret)
		return ret;

	if (ft_check_opts(ctx, FT_OPT_VERIFY_DATA | FT_OPT_ACTIVE))
	{
		ret = ft_check_buf(ctx, (char *)ctx->rx_buf + ft_rx_prefix_size(ctx), size);
		if (ret)
			return ret;
	}
	/* TODO: verify CQ data, if available */

	/* Ignore the size arg. Post a buffer large enough to handle all message
	 * sizes. ft_sync() makes use of ft_rx() and gets called in tests just before
	 * message size is updated. The recvs posted are always for the next incoming
	 * message */
	ret = ft_post_rx(ctx, ep, ctx->rx_size, &ctx->rx_ctx);
	return ret;
}

/*
 * Received messages match tagged buffers in order, but the completions can be
 * reported out of order.  A tag is valid if it's within the current window.
 */
static inline int
ft_tag_is_valid(struct ConnectionContext *ctx, struct fid_cq *cq, struct fi_cq_err_entry *comp, uint64_t tag)
{
	int valid = 1;

	if ((ctx->hints->caps & FI_TAGGED) && (cq == ctx->rxcq))
	{
		if (ctx->opts.options & FT_OPT_BW)
		{
			/* valid: (tag - window) < comp->tag < (tag + window) */
			valid = (tag < comp->tag + ctx->opts.window_size) &&
					(comp->tag < tag + ctx->opts.window_size);
		}
		else
		{
			valid = (comp->tag == tag);
		}

		if (!valid)
		{
			FT_ERR("Tag mismatch!. Expected: %" PRIu64 ", actual: %" PRIu64, tag, comp->tag);
		}
	}

	return valid;
}
/*
 * fi_cq_err_entry can be cast to any CQ entry format.
 */
static int ft_spin_for_comp(struct ConnectionContext *ctx, struct fid_cq *cq, uint64_t *cur,
							uint64_t total, int timeout)
{
	struct fi_cq_err_entry comp;
	struct timespec a, b;
	int ret;

	if (timeout >= 0)
		clock_gettime(CLOCK_MONOTONIC, &a);

	do
	{
		ret = fi_cq_read(cq, &comp, 1);
		if (ret > 0)
		{
			if (timeout >= 0)
				clock_gettime(CLOCK_MONOTONIC, &a);
			if (!ft_tag_is_valid(ctx, cq, &comp, ctx->ft_tag ? ctx->ft_tag : ctx->rx_cq_cntr))
				return -FI_EOTHER;
			(*cur)++;
		}
		else if (ret < 0 && ret != -FI_EAGAIN)
		{
			return ret;
		}
		else if (timeout >= 0)
		{
			clock_gettime(CLOCK_MONOTONIC, &b);
			if ((b.tv_sec - a.tv_sec) > timeout)
			{
				fprintf(stderr, "%ds timeout expired\n", timeout);
				return -FI_ENODATA;
			}
		}
	} while (total - *cur > 0);

	return 0;
}

/*
 * fi_cq_err_entry can be cast to any CQ entry format.
 */
static int ft_wait_for_comp(struct ConnectionContext *ctx, struct fid_cq *cq, uint64_t *cur,
							uint64_t total, int timeout)
{
	struct fi_cq_err_entry comp;
	int ret;

	while (total - *cur > 0)
	{
		ret = fi_cq_sread(cq, &comp, 1, NULL, timeout);
		if (ret > 0)
		{
			if (!ft_tag_is_valid(ctx, cq, &comp, ctx->ft_tag ? ctx->ft_tag : ctx->rx_cq_cntr))
				return -FI_EOTHER;
			(*cur)++;
		}
		else if (ret < 0 && ret != -FI_EAGAIN)
		{
			return ret;
		}
	}

	return 0;
}

/*
 * fi_cq_err_entry can be cast to any CQ entry format.
 */
static int ft_fdwait_for_comp(struct ConnectionContext *ctx, struct fid_cq *cq, uint64_t *cur,
							  uint64_t total, int timeout)
{
	struct fi_cq_err_entry comp;
	struct fid *fids[1];
	int fd, ret;

	fd = cq == ctx->txcq ? ctx->tx_fd : ctx->rx_fd;
	fids[0] = &cq->fid;

	while (total - *cur > 0)
	{
		ret = fi_trywait(ctx->fabric, fids, 1);
		if (ret == FI_SUCCESS)
		{
			ret = ft_poll_fd(fd, timeout);
			if (ret && ret != -FI_EAGAIN)
				return ret;
		}

		ret = fi_cq_read(cq, &comp, 1);
		if (ret > 0)
		{
			if (!ft_tag_is_valid(ctx, cq, &comp, ctx->ft_tag ? ctx->ft_tag : ctx->rx_cq_cntr))
				return -FI_EOTHER;
			(*cur)++;
		}
		else if (ret < 0 && ret != -FI_EAGAIN)
		{
			return ret;
		}
	}

	return 0;
}

static int ft_get_cq_comp(struct ConnectionContext *ctx, struct fid_cq *cq, uint64_t *cur,
						  uint64_t total, int timeout)
{
	int ret;

	switch (ctx->opts.comp_method)
	{
	case FT_COMP_SREAD:
	case FT_COMP_YIELD:
		ret = ft_wait_for_comp(ctx, cq, cur, total, timeout);
		break;
	case FT_COMP_WAIT_FD:
		ret = ft_fdwait_for_comp(ctx, cq, cur, total, timeout);
		break;
	default:
		ret = ft_spin_for_comp(ctx, cq, cur, total, timeout);
		break;
	}

	if (ret)
	{
		if (ret == -FI_EAVAIL)
		{
			ret = ft_cq_readerr(cq);
			(*cur)++;
		}
		else
		{
			FT_PRINTERR("ft_get_cq_comp", ret);
		}
	}
	return ret;
}

static int ft_spin_for_cntr(struct fid_cntr *cntr, uint64_t total, int timeout)
{
	struct timespec a, b;
	uint64_t cur;

	if (timeout >= 0)
		clock_gettime(CLOCK_MONOTONIC, &a);

	for (;;)
	{
		cur = fi_cntr_read(cntr);
		if (cur >= total)
			return 0;

		if (timeout >= 0)
		{
			clock_gettime(CLOCK_MONOTONIC, &b);
			if ((b.tv_sec - a.tv_sec) > timeout)
				break;
		}
	}

	fprintf(stderr, "%ds timeout expired\n", timeout);
	return -FI_ENODATA;
}

static int ft_wait_for_cntr(struct fid_cntr *cntr, uint64_t total, int timeout)
{
	int ret;

	while (fi_cntr_read(cntr) < total)
	{
		ret = fi_cntr_wait(cntr, total, timeout);
		if (ret)
			FT_PRINTERR("fi_cntr_wait", ret);
		else
			break;
	}
	return 0;
}

static int ft_get_cntr_comp(struct ConnectionContext *ctx, struct fid_cntr *cntr, uint64_t total, int timeout)
{
	int ret = 0;

	switch (ctx->opts.comp_method)
	{
	case FT_COMP_SREAD:
	case FT_COMP_WAITSET:
	case FT_COMP_WAIT_FD:
	case FT_COMP_YIELD:
		ret = ft_wait_for_cntr(cntr, total, timeout);
		break;
	default:
		ret = ft_spin_for_cntr(cntr, total, timeout);
		break;
	}

	if (ret)
		FT_PRINTERR("fs_get_cntr_comp", ret);

	return ret;
}

int ft_get_rx_comp(struct ConnectionContext *ctx, uint64_t total)
{
	int ret = FI_SUCCESS;

	if (ctx->opts.options & FT_OPT_RX_CQ)
	{
		ret = ft_get_cq_comp(ctx, ctx->rxcq, &ctx->rx_cq_cntr, total, ctx->timeout);
	}
	else if (ctx->rxcntr)
	{
		ret = ft_get_cntr_comp(ctx, ctx->rxcntr, total, ctx->timeout);
	}
	else
	{
		FT_ERR("Trying to get a RX completion when no RX CQ or counter were opened");
		ret = -FI_EOTHER;
	}
	return ret;
}

int ft_get_tx_comp(struct ConnectionContext *ctx, uint64_t total)
{
	int ret;

	if (ctx->opts.options & FT_OPT_TX_CQ)
	{
		ret = ft_get_cq_comp(ctx, ctx->txcq, &ctx->tx_cq_cntr, total, -1);
	}
	else if (ctx->txcntr)
	{
		ret = ft_get_cntr_comp(ctx, ctx->txcntr, total, -1);
	}
	else
	{
		FT_ERR("Trying to get a TX completion when no TX CQ or counter were opened");
		ret = -FI_EOTHER;
	}
	return ret;
}

int ft_sendmsg(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr,
			   size_t size, void *ctxptr, int flags)
{
	int ret;
	struct fi_msg msg;
	struct fi_msg_tagged tagged_msg;
	struct iovec msg_iov;

	msg_iov.iov_base = ctx->tx_buf;
	msg_iov.iov_len = size;

	if (ctx->hints->caps & FI_TAGGED)
	{
		tagged_msg.msg_iov = &msg_iov;
		tagged_msg.desc = &ctx->mr_desc;
		tagged_msg.iov_count = 1;
		tagged_msg.addr = fi_addr;
		tagged_msg.data = NO_CQ_DATA;
		tagged_msg.context = ctxptr;
		tagged_msg.tag = ctx->ft_tag ? ctx->ft_tag : ctx->tx_seq;
		tagged_msg.ignore = 0;

		ret = fi_tsendmsg(ep, &tagged_msg, flags);
		if (ret)
		{
			FT_PRINTERR("fi_tsendmsg", ret);
			return ret;
		}
	}
	else
	{
		msg.msg_iov = &msg_iov;
		msg.desc = &ctx->mr_desc;
		msg.iov_count = 1;
		msg.addr = fi_addr;
		msg.data = NO_CQ_DATA;
		msg.context = ctxptr;

		ret = fi_sendmsg(ep, &msg, flags);
		if (ret)
		{
			FT_PRINTERR("fi_sendmsg", ret);
			return ret;
		}
	}
	ctx->tx_seq++;
	return 0;
}

int ft_recvmsg(struct ConnectionContext *ctx, struct fid_ep *ep, fi_addr_t fi_addr,
			   size_t size, void *ctxptr, int flags)
{
	int ret;
	struct fi_msg msg;
	struct fi_msg_tagged tagged_msg;
	struct iovec msg_iov;

	msg_iov.iov_base = ctx->rx_buf;
	msg_iov.iov_len = size;

	if (ctx->hints->caps & FI_TAGGED)
	{
		tagged_msg.msg_iov = &msg_iov;
		tagged_msg.desc = &ctx->mr_desc;
		tagged_msg.iov_count = 1;
		tagged_msg.addr = fi_addr;
		tagged_msg.data = NO_CQ_DATA;
		tagged_msg.context = ctxptr;
		tagged_msg.tag = ctx->ft_tag ? ctx->ft_tag : ctx->tx_seq;
		tagged_msg.ignore = 0;

		ret = fi_trecvmsg(ep, &tagged_msg, flags);
		if (ret)
		{
			FT_PRINTERR("fi_trecvmsg", ret);
			return ret;
		}
	}
	else
	{
		msg.msg_iov = &msg_iov;
		msg.desc = &ctx->mr_desc;
		msg.iov_count = 1;
		msg.addr = fi_addr;
		msg.data = NO_CQ_DATA;
		msg.context = ctxptr;

		ret = fi_recvmsg(ep, &msg, flags);
		if (ret)
		{
			FT_PRINTERR("fi_recvmsg", ret);
			return ret;
		}
	}

	return 0;
}

int ft_cq_read_verify(struct ConnectionContext *ctx, struct fid_cq *cq, void *op_context)
{
	int ret;
	struct fi_cq_err_entry completion;

	do
	{
		/* read events from the completion queue */
		ret = fi_cq_read(cq, (void *)&completion, 1);

		if (ret > 0)
		{
			if (op_context != completion.op_context)
			{
				fprintf(stderr, "ERROR: op ctx=%p cq_ctx=%p\n",
						op_context, completion.op_context);
				return -FI_EOTHER;
			}
			if (!ft_tag_is_valid(ctx, cq, &completion,
								 ctx->ft_tag ? ctx->ft_tag : ctx->rx_cq_cntr))
				return -FI_EOTHER;
		}
		else if ((ret <= 0) && (ret != -FI_EAGAIN))
		{
			FT_PRINTERR("POLL: Error\n", ret);
			if (ret == -FI_EAVAIL)
				FT_PRINTERR("POLL: error available\n", ret);
			return -FI_EOTHER;
		}
	} while (ret == -FI_EAGAIN);

	return 0;
}

int ft_cq_readerr(struct fid_cq *cq)
{
	struct fi_cq_err_entry cq_err;
	int ret;

	memset(&cq_err, 0, sizeof(cq_err));
	ret = fi_cq_readerr(cq, &cq_err, 0);
	if (ret < 0)
	{
		FT_PRINTERR("fi_cq_readerr", ret);
	}
	else
	{
		FT_CQ_ERR(cq, cq_err, NULL, 0);
		ret = -cq_err.err;
	}
	return ret;
}

void eq_readerr(struct fid_eq *eq, const char *eq_str)
{
	struct fi_eq_err_entry eq_err;
	int rd;

	memset(&eq_err, 0, sizeof(eq_err));
	rd = fi_eq_readerr(eq, &eq_err, 0);
	if (rd != sizeof(eq_err))
	{
		FT_PRINTERR("fi_eq_readerr", rd);
	}
	else
	{
		FT_EQ_ERR(eq, eq_err, NULL, 0);
	}
}

int ft_sync(struct ConnectionContext *ctx)
{
	char buf;
	int ret;

	if (ctx->opts.dst_addr)
	{
		if (!(ctx->opts.options & FT_OPT_OOB_SYNC))
		{
			ret = ft_tx(ctx, ctx->ep, ctx->remote_fi_addr, 1, &ctx->tx_ctx);
			if (ret)
				return ret;

			ret = ft_rx(ctx, ctx->ep, 1);
		}
		else
		{
			ret = ft_sock_send(ctx, ctx->oob_sock, &buf, 1);
			if (ret)
				return ret;

			ret = ft_sock_recv(ctx, ctx->oob_sock, &buf, 1);
			if (ret)
				return ret;
		}
	}
	else
	{
		if (!(ctx->opts.options & FT_OPT_OOB_SYNC))
		{
			ret = ft_rx(ctx, ctx->ep, 1);
			if (ret)
				return ret;

			ret = ft_tx(ctx, ctx->ep, ctx->remote_fi_addr, 1, &ctx->tx_ctx);
		}
		else
		{
			ret = ft_sock_recv(ctx, ctx->oob_sock, &buf, 1);
			if (ret)
				return ret;

			ret = ft_sock_send(ctx, ctx->oob_sock, &buf, 1);
			if (ret)
				return ret;
		}
	}

	return ret;
}

int ft_sync_pair(int status)
{
	int ret;
	int pair_status;

	if (ft_parent_proc)
	{
		ret = write(ft_socket_pair[1], &status, sizeof(int));
		if (ret < 0)
		{
			FT_PRINTERR("write", errno);
			return ret;
		}
		ret = read(ft_socket_pair[1], &pair_status, sizeof(int));
		if (ret < 0)
		{
			FT_PRINTERR("read", errno);
			return ret;
		}
	}
	else
	{
		ret = read(ft_socket_pair[0], &pair_status, sizeof(int));
		if (ret < 0)
		{
			FT_PRINTERR("read", errno);
			return ret;
		}
		ret = write(ft_socket_pair[0], &status, sizeof(int));
		if (ret < 0)
		{
			FT_PRINTERR("write", errno);
			return ret;
		}
	}

	/* check status reported the other guy */
	if (pair_status != FI_SUCCESS)
		return pair_status;

	return 0;
}

int ft_fork_and_pair(void)
{
	int ret;

	ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, ft_socket_pair);
	if (ret)
	{
		FT_PRINTERR("socketpair", errno);
		return -errno;
	}

	ft_child_pid = fork();
	if (ft_child_pid < 0)
	{
		FT_PRINTERR("fork", ft_child_pid);
		return -errno;
	}
	if (ft_child_pid)
		ft_parent_proc = 1;

	return 0;
}

int ft_fork_child(void)
{
	ft_child_pid = fork();
	if (ft_child_pid < 0)
	{
		FT_PRINTERR("fork", ft_child_pid);
		return -errno;
	}

	if (ft_child_pid == 0)
	{
		exit(0);
	}

	return 0;
}

int ft_wait_child(void)
{
	int ret;

	ret = close(ft_socket_pair[0]);
	if (ret)
	{
		FT_PRINTERR("close", errno);
		return ret;
	}
	ret = close(ft_socket_pair[1]);
	if (ret)
	{
		FT_PRINTERR("close", errno);
		return ret;
	}
	if (ft_parent_proc)
	{
		ret = waitpid(ft_child_pid, NULL, WCONTINUED);
		if (ret < 0)
		{
			FT_PRINTERR("waitpid", errno);
			return ret;
		}
	}

	return 0;
}

int ft_finalize_ep(struct ConnectionContext *ctx, struct fid_ep *ep)
{
	struct iovec iov;
	int ret;
	struct fi_context ctxptr;

	iov.iov_base = ctx->tx_buf;
	iov.iov_len = 4 + ft_tx_prefix_size(ctx);

	if (ctx->hints->caps & FI_TAGGED)
	{
		struct fi_msg_tagged tmsg;

		memset(&tmsg, 0, sizeof tmsg);
		tmsg.msg_iov = &iov;
		tmsg.desc = &ctx->mr_desc;
		tmsg.iov_count = 1;
		tmsg.addr = ctx->remote_fi_addr;
		tmsg.tag = ctx->tx_seq;
		tmsg.ignore = 0;
		tmsg.context = &ctxptr;

		FT_POST(fi_tsendmsg, ft_progress, ctx->txcq, ctx->tx_seq,
				&ctx->tx_cq_cntr, "tsendmsg", ep, &tmsg,
				FI_TRANSMIT_COMPLETE);
	}
	else
	{
		struct fi_msg msg;

		memset(&msg, 0, sizeof msg);
		msg.msg_iov = &iov;
		msg.desc = &ctx->mr_desc;
		msg.iov_count = 1;
		msg.addr = ctx->remote_fi_addr;
		msg.context = &ctxptr;

		FT_POST(fi_sendmsg, ft_progress, ctx->txcq, ctx->tx_seq,
				&ctx->tx_cq_cntr, "sendmsg", ep, &msg,
				FI_TRANSMIT_COMPLETE);
	}

	ret = ft_get_tx_comp(ctx, ctx->tx_seq);
	if (ret)
		return ret;

	ret = ft_get_rx_comp(ctx, ctx->rx_seq);
	if (ret)
		return ret;

	return 0;
}

int ft_finalize(struct ConnectionContext *ctx)
{
	int ret;

	if (ctx->fi->domain_attr->mr_mode & FI_MR_RAW)
	{
		ret = fi_mr_unmap_key(ctx->domain, ctx->remote.key);
		if (ret)
			return ret;
	}

	return ft_finalize_ep(ctx, ctx->ep);
}

int64_t get_elapsed(const struct timespec *b, const struct timespec *a,
					enum precision p)
{
	int64_t elapsed;

	elapsed = difftime(a->tv_sec, b->tv_sec) * 1000 * 1000 * 1000;
	elapsed += a->tv_nsec - b->tv_nsec;
	return elapsed / p;
}

void show_perf(char *name, size_t tsize, int iters, struct timespec *start,
			   struct timespec *end, int xfers_per_iter)
{
	static int header = 1;
	char str[FT_STR_LEN];
	int64_t elapsed = get_elapsed(start, end, MICRO);
	long long bytes = (long long)iters * tsize * xfers_per_iter;
	float usec_per_xfer;

	if (name)
	{
		if (header)
		{
			printf("%-50s%-8s%-8s%-8s%8s %10s%13s%13s\n",
				   "name", "bytes", "iters",
				   "total", "time", "MB/sec",
				   "usec/xfer", "Mxfers/sec");
			header = 0;
		}

		printf("%-50s", name);
	}
	else
	{
		if (header)
		{
			printf("%-8s%-8s%-8s%8s %10s%13s%13s\n",
				   "bytes", "iters", "total",
				   "time", "MB/sec", "usec/xfer",
				   "Mxfers/sec");
			header = 0;
		}
	}

	printf("%-8s", size_str(str, tsize));

	printf("%-8s", cnt_str(str, iters));

	printf("%-8s", size_str(str, bytes));

	usec_per_xfer = ((float)elapsed / iters / xfers_per_iter);
	printf("%8.2fs%10.2f%11.2f%11.2f\n",
		   elapsed / 1000000.0, bytes / (1.0 * elapsed),
		   usec_per_xfer, 1.0 / usec_per_xfer);
}

void show_perf_mr(size_t tsize, int iters, struct timespec *start,
				  struct timespec *end, int xfers_per_iter, int argc, char *argv[])
{
	static int header = 1;
	int64_t elapsed = get_elapsed(start, end, MICRO);
	long long total = (long long)iters * tsize * xfers_per_iter;
	int i;
	float usec_per_xfer;

	if (header)
	{
		printf("---\n");

		for (i = 0; i < argc; ++i)
			printf("%s ", argv[i]);

		printf(":\n");
		header = 0;
	}

	usec_per_xfer = ((float)elapsed / iters / xfers_per_iter);

	printf("- { ");
	printf("xfer_size: %zu, ", tsize);
	printf("iterations: %d, ", iters);
	printf("total: %lld, ", total);
	printf("time: %f, ", elapsed / 1000000.0);
	printf("MB/sec: %f, ", (total) / (1.0 * elapsed));
	printf("usec/xfer: %f, ", usec_per_xfer);
	printf("Mxfers/sec: %f", 1.0 / usec_per_xfer);
	printf(" }\n");
}

void ft_addr_usage()
{
	FT_PRINT_OPTS_USAGE("-B <src_port>", "non default source port number");
	FT_PRINT_OPTS_USAGE("-P <dst_port>", "non default destination port number");
	FT_PRINT_OPTS_USAGE("-s <address>", "source address");
	FT_PRINT_OPTS_USAGE("-b[=<oob_port>]", "enable out-of-band address exchange and "
										   "synchronization over the, optional, port");
	FT_PRINT_OPTS_USAGE("-E[=<oob_port>]", "enable out-of-band address exchange only "
										   "over the, optional, port");
	FT_PRINT_OPTS_USAGE("-C <number>", "number of connections to accept before "
									   "cleaning up a server");
	FT_PRINT_OPTS_USAGE("-F <addr_format>", "Address format (default:FI_FORMAT_UNSPEC)");
}

void ft_usage(char *name, char *desc)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s [OPTIONS]\t\tstart server\n", name);
	fprintf(stderr, "  %s [OPTIONS] <host>\tconnect to server\n", name);

	if (desc)
		fprintf(stderr, "\n%s\n", desc);

	fprintf(stderr, "\nOptions:\n");
	ft_addr_usage();
	FT_PRINT_OPTS_USAGE("-f <fabric>", "fabric name");
	FT_PRINT_OPTS_USAGE("-d <domain>", "domain name");
	FT_PRINT_OPTS_USAGE("-p <provider>", "specific provider name eg sockets, verbs");
	FT_PRINT_OPTS_USAGE("-e <ep_type>", "Endpoint type: msg|rdm|dgram (default:rdm)");
	FT_PRINT_OPTS_USAGE("", "Only the following tests support this option for now:");
	FT_PRINT_OPTS_USAGE("", "fi_rma_bw");
	FT_PRINT_OPTS_USAGE("", "fi_shared_ctx");
	FT_PRINT_OPTS_USAGE("", "fi_multi_mr");
	FT_PRINT_OPTS_USAGE("", "fi_multi_ep");
	FT_PRINT_OPTS_USAGE("", "fi_recv_cancel");
	FT_PRINT_OPTS_USAGE("", "fi_unexpected_msg");
	FT_PRINT_OPTS_USAGE("", "fi_resmgmt_test");
	FT_PRINT_OPTS_USAGE("", "fi_inj_complete");
	FT_PRINT_OPTS_USAGE("", "fi_bw");
	FT_PRINT_OPTS_USAGE("-U", "run fabtests with FI_DELIVERY_COMPLETE set");
	FT_PRINT_OPTS_USAGE("", "Only the following tests support this option for now:");
	FT_PRINT_OPTS_USAGE("", "fi_bw");
	FT_PRINT_OPTS_USAGE("", "fi_rdm");
	FT_PRINT_OPTS_USAGE("", "fi_rdm_atomic");
	FT_PRINT_OPTS_USAGE("", "fi_rdm_pingpong");
	FT_PRINT_OPTS_USAGE("", "fi_rdm_tagged_bw");
	FT_PRINT_OPTS_USAGE("", "fi_rdm_tagged_pingpong");
	FT_PRINT_OPTS_USAGE("", "fi_rma_bw");
	FT_PRINT_OPTS_USAGE("-M <mode>", "Disable mode bit from test");
	FT_PRINT_OPTS_USAGE("-K", "fork a child process after initializing endpoint");
	FT_PRINT_OPTS_USAGE("", "mr_local");
	FT_PRINT_OPTS_USAGE("-a <address vector name>", "name of address vector");
	FT_PRINT_OPTS_USAGE("-h", "display this help output");

	return;
}

void ft_mcusage(char *name, char *desc)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s [OPTIONS] -M <mcast_addr>\tstart listener\n", name);
	fprintf(stderr, "  %s [OPTIONS] <mcast_addr>\tsend to group\n", name);

	if (desc)
		fprintf(stderr, "\n%s\n", desc);

	fprintf(stderr, "\nOptions:\n");
	ft_addr_usage();
	FT_PRINT_OPTS_USAGE("-f <fabric>", "fabric name");
	FT_PRINT_OPTS_USAGE("-d <domain>", "domain name");
	FT_PRINT_OPTS_USAGE("-p <provider>", "specific provider name eg sockets, verbs");
	FT_PRINT_OPTS_USAGE("-d <domain>", "domain name");
	FT_PRINT_OPTS_USAGE("-p <provider>", "specific provider name eg sockets, verbs");
	FT_PRINT_OPTS_USAGE("-D <device_iface>", "Specify device interface: eg ze (default: None). "
											 "Automatically enables FI_HMEM (-H)");
	FT_PRINT_OPTS_USAGE("-i <device_id>", "Specify which device to use (default: 0)");
	FT_PRINT_OPTS_USAGE("-H", "Enable provider FI_HMEM support");
	FT_PRINT_OPTS_USAGE("-h", "display this help output");

	return;
}

void ft_csusage(char *name, char *desc)
{
	ft_usage(name, desc);
	FT_PRINT_OPTS_USAGE("-I <number>", "number of iterations");
	FT_PRINT_OPTS_USAGE("-Q", "bind EQ to domain (vs. endpoint)");
	FT_PRINT_OPTS_USAGE("-w <number>", "number of warmup iterations");
	FT_PRINT_OPTS_USAGE("-S <size>", "specific transfer size or 'all'");
	FT_PRINT_OPTS_USAGE("-l", "align transmit and receive buffers to page size");
	FT_PRINT_OPTS_USAGE("-m", "machine readable output");
	FT_PRINT_OPTS_USAGE("-D <device_iface>", "Specify device interface: eg cuda, ze(default: None). "
											 "Automatically enables FI_HMEM (-H)");
	FT_PRINT_OPTS_USAGE("-t <type>", "completion type [queue, counter]");
	FT_PRINT_OPTS_USAGE("-c <method>", "completion method [spin, sread, fd, yield]");
	FT_PRINT_OPTS_USAGE("-h", "display this help output");

	return;
}

void ft_parseinfo(int op, char *optarg, struct fi_info *hints,
				  struct ft_opts *opts)
{
	switch (op)
	{
	case 'f':
		if (!hints->fabric_attr)
		{
			hints->fabric_attr = malloc(sizeof *(hints->fabric_attr));
			if (!hints->fabric_attr)
			{
				perror("malloc");
				exit(EXIT_FAILURE);
			}
		}
		hints->fabric_attr->name = strdup(optarg);
		break;
	case 'd':
		if (!hints->domain_attr)
		{
			hints->domain_attr = malloc(sizeof *(hints->domain_attr));
			if (!hints->domain_attr)
			{
				perror("malloc");
				exit(EXIT_FAILURE);
			}
		}
		hints->domain_attr->name = strdup(optarg);
		break;
	case 'p':
		if (!hints->fabric_attr)
		{
			hints->fabric_attr = malloc(sizeof *(hints->fabric_attr));
			if (!hints->fabric_attr)
			{
				perror("malloc");
				exit(EXIT_FAILURE);
			}
		}
		hints->fabric_attr->prov_name = strdup(optarg);
		break;
	case 'e':
		if (!strncasecmp("msg", optarg, 3))
			hints->ep_attr->type = FI_EP_MSG;
		if (!strncasecmp("rdm", optarg, 3))
			hints->ep_attr->type = FI_EP_RDM;
		if (!strncasecmp("dgram", optarg, 5))
			hints->ep_attr->type = FI_EP_DGRAM;
		break;
	case 'M':
		if (!strncasecmp("mr_local", optarg, 8))
			opts->mr_mode &= ~FI_MR_LOCAL;
		break;
	case 'D':
		if (!strncasecmp("ze", optarg, 2))
			opts->iface = FI_HMEM_ZE;
		else if (!strncasecmp("cuda", optarg, 4))
			opts->iface = FI_HMEM_CUDA;
		else
			printf("Unsupported interface\n");
		opts->options |= FT_OPT_ENABLE_HMEM | FT_OPT_USE_DEVICE;
		break;
	case 'i':
		opts->device = atoi(optarg);
		break;
	case 'H':
		opts->options |= FT_OPT_ENABLE_HMEM;
		break;
	case 'K':
		opts->options |= FT_OPT_FORK_CHILD;
		break;
	default:
		/* let getopt handle unknown opts*/
		break;
	}
}

void ft_parse_addr_opts(int op, char *optarg, struct ft_opts *opts)
{
	switch (op)
	{
	case 's':
		opts->src_addr = optarg;
		break;
	case 'B':
		opts->src_port = optarg;
		break;
	case 'P':
		opts->dst_port = optarg;
		break;
	case 'b':
		opts->options |= FT_OPT_OOB_SYNC;
		/* fall through */
	case 'E':
		opts->options |= FT_OPT_OOB_ADDR_EXCH;
		if (optarg && strlen(optarg) > 1)
			opts->oob_port = optarg + 1;
		else
			opts->oob_port = default_oob_port;
		break;
	case 'F':
		if (!strncasecmp("fi_sockaddr_in6", optarg, 15))
			opts->address_format = FI_SOCKADDR_IN6;
		else if (!strncasecmp("fi_sockaddr_in", optarg, 14))
			opts->address_format = FI_SOCKADDR_IN;
		else if (!strncasecmp("fi_sockaddr_ib", optarg, 14))
			opts->address_format = FI_SOCKADDR_IB;
		else if (!strncasecmp("fi_sockaddr", optarg, 11)) /* keep me last */
			opts->address_format = FI_SOCKADDR;
		break;
	case 'C':
		opts->options |= FT_OPT_SERVER_PERSIST;
		opts->num_connections = atoi(optarg);
	default:
		/* let getopt handle unknown opts*/
		break;
	}
}

void ft_parsecsopts(int op, char *optarg, struct ft_opts *opts)
{
	ft_parse_addr_opts(op, optarg, opts);

	switch (op)
	{
	case 'I':
		opts->options |= FT_OPT_ITER;
		opts->iterations = atoi(optarg);
		break;
	case 'Q':
		opts->options |= FT_OPT_DOMAIN_EQ;
		break;
	case 'S':
		if (!strncasecmp("all", optarg, 3))
		{
			opts->sizes_enabled = FT_ENABLE_ALL;
		}
		else
		{
			opts->options |= FT_OPT_SIZE;
			opts->transfer_size = atol(optarg);
		}
		break;
	case 'm':
		opts->machr = 1;
		break;
	case 'c':
		if (!strncasecmp("sread", optarg, 5))
			opts->comp_method = FT_COMP_SREAD;
		else if (!strncasecmp("fd", optarg, 2))
			opts->comp_method = FT_COMP_WAIT_FD;
		else if (!strncasecmp("yield", optarg, 5))
			opts->comp_method = FT_COMP_YIELD;
		break;
	case 't':
		if (!strncasecmp("counter", optarg, 7))
		{
			opts->options |= FT_OPT_RX_CNTR | FT_OPT_TX_CNTR;
			opts->options &= ~(FT_OPT_RX_CQ | FT_OPT_TX_CQ);
		}
		break;
	case 'a':
		opts->av_name = optarg;
		break;
	case 'w':
		opts->warmup_iterations = atoi(optarg);
		break;
	case 'l':
		opts->options |= FT_OPT_ALIGN;
		break;
	default:
		/* let getopt handle unknown opts*/
		break;
	}
}

int ft_parse_rma_opts(struct ConnectionContext *ctx, int op, char *optarg, struct fi_info *hints,
					  struct ft_opts *opts)
{
	switch (op)
	{
	case 'o':
		if (!strcmp(optarg, "read"))
		{
			hints->caps |= FI_READ | FI_REMOTE_READ;
			opts->rma_op = FT_RMA_READ;
		}
		else if (!strcmp(optarg, "writedata"))
		{
			hints->caps |= FI_WRITE | FI_REMOTE_WRITE;
			hints->mode |= FI_RX_CQ_DATA;
			hints->domain_attr->cq_data_size = 4;
			opts->rma_op = FT_RMA_WRITEDATA;
			ctx->cq_attr.format = FI_CQ_FORMAT_DATA;
		}
		else if (!strcmp(optarg, "write"))
		{
			hints->caps |= FI_WRITE | FI_REMOTE_WRITE;
			opts->rma_op = FT_RMA_WRITE;
		}
		else
		{
			fprintf(stderr, "Invalid operation type: \"%s\". Usage:\n"
							"-o <op>\trma op type: read|write|writedata "
							"(default:write)\n",
					optarg);
			return EXIT_FAILURE;
		}
		break;
	default:
		/* let getopt handle unknown opts*/
		break;
	}
	return 0;
}

int ft_fill_buf(struct ConnectionContext *ctx, void *buf, size_t size)
{
	char *msg_buf;
	int msg_index = 0;
	size_t i;
	int ret = 0;

	if (ctx->opts.iface != FI_HMEM_SYSTEM)
	{
		msg_buf = malloc(size);
		if (!msg_buf)
			return -FI_ENOMEM;
	}
	else
	{
		msg_buf = (char *)buf;
	}

	for (i = 0; i < size; i++)
	{
		msg_buf[i] = integ_alphabet[msg_index];
		if (++msg_index >= integ_alphabet_length)
			msg_index = 0;
	}

	if (ctx->opts.iface != FI_HMEM_SYSTEM)
	{
		ret = ft_hmem_copy_to(ctx->opts.iface, ctx->opts.device, buf, msg_buf, size);
		if (ret)
			goto out;
	}
out:
	if (ctx->opts.iface != FI_HMEM_SYSTEM)
		free(msg_buf);
	return ret;
}

int ft_check_buf(struct ConnectionContext *ctx, void *buf, size_t size)
{
	char *recv_data;
	char c;
	int msg_index = 0;
	size_t i;
	int ret = 0;

	if (ctx->opts.iface != FI_HMEM_SYSTEM)
	{
		recv_data = malloc(size);
		if (!recv_data)
			return -FI_ENOMEM;

		ret = ft_hmem_copy_from(ctx->opts.iface, ctx->opts.device,
								recv_data, buf, size);
		if (ret)
			goto out;
	}
	else
	{
		recv_data = (char *)buf;
	}

	for (i = 0; i < size; i++)
	{
		c = integ_alphabet[msg_index];
		if (++msg_index >= integ_alphabet_length)
			msg_index = 0;
		if (c != recv_data[i])
			break;
	}
	if (i != size)
	{
		printf("Data check error (%c!=%c) at byte %zu for "
			   "buffer size %zu\n",
			   c, recv_data[i], i, size);
		ret = -FI_EIO;
	}

out:
	if (ctx->opts.iface != FI_HMEM_SYSTEM)
		free(recv_data);
	return ret;
}

uint64_t ft_init_cq_data(struct fi_info *info)
{
	if (info->domain_attr->cq_data_size >= sizeof(uint64_t))
	{
		return 0x0123456789abcdefULL;
	}
	else
	{
		return 0x0123456789abcdef &
			   ((0x1ULL << (info->domain_attr->cq_data_size * 8)) - 1);
	}
}

int check_recv_msg(struct ConnectionContext *ctx, const char *message)
{
	size_t recv_len;
	size_t message_len = strlen(message) + 1;
	/* Account for null terminated byte. */
	recv_len = strlen(ctx->rx_buf) + 1;

	if (recv_len != message_len)
	{
		fprintf(stderr, "Received length does not match expected length.\n");
		return -1;
	}

	if (strncmp(ctx->rx_buf, message, message_len))
	{
		fprintf(stderr, "Received message does not match expected message.\n");
		return -1;
	}
	fprintf(stdout, "Data check OK\n");
	return 0;
}

int ft_send_greeting(struct ConnectionContext *ctx, struct fid_ep *ep)
{
	size_t message_len = strlen(greeting) + 1;
	int ret;

	fprintf(stdout, "Sending message...\n");
	if (snprintf(ctx->tx_buf, ctx->tx_size, "%s", greeting) >= ctx->tx_size)
	{
		fprintf(stderr, "Transmit buffer too small.\n");
		return -FI_ETOOSMALL;
	}

	ret = ft_tx(ctx, ep, ctx->remote_fi_addr, message_len, &ctx->tx_ctx);
	if (ret)
		return ret;

	fprintf(stdout, "Send completion received\n");
	return 0;
}

int ft_recv_greeting(struct ConnectionContext *ctx, struct fid_ep *ep)
{
	int ret;

	fprintf(stdout, "Waiting for message from client...\n");
	ret = ft_get_rx_comp(ctx, ctx->rx_seq);
	if (ret)
		return ret;

	ret = check_recv_msg(ctx, greeting);
	if (ret)
		return ret;

	fprintf(stdout, "Received data from client: %s\n", (char *)ctx->rx_buf);
	return 0;
}

int ft_send_recv_greeting(struct ConnectionContext *ctx, struct fid_ep *ep)
{
	return ctx->opts.dst_addr ? ft_send_greeting(ctx, ep) : ft_recv_greeting(ctx, ep);
}

int ft_sock_listen(struct ConnectionContext *ctx, char *node, char *service)
{
	struct addrinfo *ai, hints;
	int val, ret;

	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;

	ret = getaddrinfo(node, service, &hints, &ai);
	if (ret)
	{
		fprintf(stderr, "getaddrinfo() %s\n", gai_strerror(ret));
		return ret;
	}

	ctx->listen_sock = socket(ai->ai_family, SOCK_STREAM, 0);
	if (ctx->listen_sock < 0)
	{
		perror("socket");
		ret = ctx->listen_sock;
		goto out;
	}

	val = 1;
	ret = setsockopt(ctx->listen_sock, SOL_SOCKET, SO_REUSEADDR,
					 (void *)&val, sizeof val);
	if (ret)
	{
		perror("setsockopt SO_REUSEADDR");
		goto out;
	}

	ret = bind(ctx->listen_sock, ai->ai_addr, ai->ai_addrlen);
	if (ret)
	{
		perror("bind");
		goto out;
	}

	ret = listen(ctx->listen_sock, 0);
	if (ret)
		perror("listen");

out:
	if (ret && ctx->listen_sock >= 0)
		close(ctx->listen_sock);
	freeaddrinfo(ai);
	return ret;
}

int ft_sock_connect(struct ConnectionContext *ctx, char *node, char *service)
{
	struct addrinfo *ai;
	int ret;

	ret = getaddrinfo(node, service, NULL, &ai);
	if (ret)
	{
		perror("getaddrinfo");
		return ret;
	}

	ctx->sock = socket(ai->ai_family, SOCK_STREAM, 0);
	if (ctx->sock < 0)
	{
		perror("socket");
		ret = ctx->sock;
		goto free;
	}

	ret = connect(ctx->sock, ai->ai_addr, ai->ai_addrlen);
	if (ret)
	{
		perror("connect");
		close(ctx->sock);
		goto free;
	}

	ret = ft_sock_setup(ctx->sock);
free:
	freeaddrinfo(ai);
	return ret;
}

int ft_sock_accept(struct ConnectionContext *ctx)
{
	int ret;

	ctx->sock = accept(ctx->listen_sock, NULL, 0);
	if (ctx->sock < 0)
	{
		ret = ctx->sock;
		perror("accept");
		return ret;
	}

	ret = ft_sock_setup(ctx->sock);
	return ret;
}

int ft_sock_send(struct ConnectionContext *ctx, int fd, void *msg, size_t len)
{
	size_t sent;
	ssize_t ret, err = 0;

	for (sent = 0; sent < len;)
	{
		ret = send(fd, ((char *)msg) + sent, len - sent, 0);
		if (ret > 0)
		{
			sent += ret;
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			ft_force_progress(ctx);
		}
		else
		{
			err = -errno;
			break;
		}
	}

	return err ? err : 0;
}

int ft_sock_recv(struct ConnectionContext *ctx, int fd, void *msg, size_t len)
{
	size_t rcvd;
	ssize_t ret, err = 0;

	for (rcvd = 0; rcvd < len;)
	{
		ret = recv(fd, ((char *)msg) + rcvd, len - rcvd, 0);
		if (ret > 0)
		{
			rcvd += ret;
		}
		else if (ret == 0)
		{
			err = -FI_ENOTCONN;
			break;
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			ft_force_progress(ctx);
		}
		else
		{
			err = -errno;
			break;
		}
	}

	return err ? err : 0;
}

int ft_sock_sync(struct ConnectionContext *ctx, int value)
{
	int result = -FI_EOTHER;

	if (ctx->listen_sock < 0)
	{
		ft_sock_send(ctx, ctx->sock, &value, sizeof value);
		ft_sock_recv(ctx, ctx->sock, &result, sizeof result);
	}
	else
	{
		ft_sock_recv(ctx, ctx->sock, &result, sizeof result);
		ft_sock_send(ctx, ctx->sock, &value, sizeof value);
	}

	return result;
}

void ft_sock_shutdown(int fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
}

static int ft_has_util_prefix(const char *str)
{
	return !strncasecmp(str, OFI_UTIL_PREFIX, strlen(OFI_UTIL_PREFIX));
}

const char *ft_util_name(const char *str, size_t *len)
{
	char *delim;

	delim = strchr(str, OFI_NAME_DELIM);
	if (delim)
	{
		if (ft_has_util_prefix(delim + 1))
		{
			*len = strlen(delim + 1);
			return delim + 1;
		}
		else if (ft_has_util_prefix(str))
		{
			*len = delim - str;
			return str;
		}
	}
	else if (ft_has_util_prefix(str))
	{
		*len = strlen(str);
		return str;
	}
	*len = 0;
	return NULL;
}

const char *ft_core_name(const char *str, size_t *len)
{
	char *delim;

	delim = strchr(str, OFI_NAME_DELIM);
	if (delim)
	{
		if (!ft_has_util_prefix(delim + 1))
		{
			*len = strlen(delim + 1);
			return delim + 1;
		}
		else if (!ft_has_util_prefix(str))
		{
			*len = delim - str;
			return str;
		}
	}
	else if (!ft_has_util_prefix(str))
	{
		*len = strlen(str);
		return str;
	}
	*len = 0;
	return NULL;
}

/* Split the given string "s" using the specified delimiter(s) in the string
 * "delim" and return an array of strings. The array is terminated with a NULL
 * pointer. Returned array should be freed with ft_free_string_array().
 *
 * Returns NULL on failure.
 */

char **ft_split_and_alloc(const char *s, const char *delim, size_t *count)
{
	int i, n;
	char *tmp;
	char *dup = NULL;
	char **arr = NULL;

	if (!s || !delim)
		return NULL;

	dup = strdup(s);
	if (!dup)
		return NULL;

	/* compute the array size */
	n = 1;
	for (tmp = dup; *tmp != '\0'; ++tmp)
	{
		for (i = 0; delim[i] != '\0'; ++i)
		{
			if (*tmp == delim[i])
			{
				++n;
				break;
			}
		}
	}

	/* +1 to leave space for NULL terminating pointer */
	arr = calloc(n + 1, sizeof(*arr));
	if (!arr)
		goto cleanup;

	/* set array elts to point inside the dup'ed string */
	for (tmp = dup, i = 0; tmp != NULL; ++i)
	{
		arr[i] = strsep(&tmp, delim);
	}
	assert(i == n);

	if (count)
		*count = n;
	return arr;

cleanup:
	free(dup);
	free(arr);
	return NULL;
}

/* see ft_split_and_alloc() */
void ft_free_string_array(char **s)
{
	/* all strings are allocated from the same strdup'ed slab, so just free
	 * the first element */
	if (s != NULL)
		free(s[0]);

	/* and then the actual array of pointers */
	free(s);
}

ssize_t ft_post_rma_selective_comp(struct ConnectionContext *ctx, enum ft_rma_opcodes op, struct fid_ep *ep, size_t size,
								   struct fi_rma_iov *remote, void *context, bool enable_completion)
{
	struct fi_msg_rma rma_msg;
	memset(&rma_msg, 0, sizeof(rma_msg));

	struct iovec iov;
	iov.iov_base = (void *)ctx->tx_buf;
	iov.iov_len = ctx->opts.transfer_size;

	struct fi_rma_iov rma_iov;
	rma_iov.addr = remote->addr;
	rma_iov.len = iov.iov_len;
	rma_iov.key = remote->key;

	rma_msg.addr = ctx->remote_fi_addr;
	rma_msg.desc = &ctx->mr_desc;
	rma_msg.context = context;
	rma_msg.msg_iov = &iov;
	rma_msg.iov_count = 1;
	rma_msg.rma_iov = &rma_iov;
	rma_msg.rma_iov_count = 1;

	uint64_t flags = enable_completion ? FI_COMPLETION : 0;

	if (op == FT_RMA_WRITE)
	{
		FT_POST(fi_writemsg, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"fi_writemsg", ep, &rma_msg, flags);
	}
	else if (op == FT_RMA_READ)
	{
		FT_POST(fi_readmsg, ft_progress, ctx->txcq, ctx->tx_seq, &ctx->tx_cq_cntr,
				"fi_readmsg", ep, &rma_msg, flags);
	}
	else
	{
		FT_ERR("Unknown RMA op type\n");
		return EXIT_FAILURE;
	}
	if (!enable_completion)
	{
		// No completion will be generated for this request. Manually increment completion cntr
		ctx->tx_cq_cntr++;
	}
	return EXIT_SUCCESS;
}

int alloc_ep_res_multi_recv(struct ConnectionContext *ctx)
{
	int ret;

	ctx->tx_size = ctx->opts.transfer_size;
	if (ctx->tx_size > ctx->fi->ep_attr->max_msg_size)
	{
		fprintf(stderr, "transfer size is larger than the maximum size "
						"of the data transfer supported by the provider\n");
		return -1;
	}

	ctx->tx_buf = malloc(ctx->tx_size);
	if (!ctx->tx_buf)
	{
		fprintf(stderr, "Cannot allocate tx_buf\n");
		return -1;
	}

	ret = fi_mr_reg(ctx->domain, ctx->tx_buf, ctx->tx_size, FI_SEND,
					0, FT_MR_KEY, 0, &ctx->mr, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_mr_reg", ret);
		return ret;
	}

	/* We only ues the common code to send messages, so
	 * set mr_desc to the tx buffer's region.
	 */
	ctx->mr_desc = fi_mr_desc(ctx->mr);

	//Each multi recv buffer will be able to hold at least 2 and
	//up to 64 messages, allowing proper testing of multi recv
	//completions and reposting
	ctx->rx_size = MIN(ctx->tx_size * 128, MAX_XFER_SIZE * 4);
	ctx->comp_per_buf = ctx->rx_size / 2 / ctx->opts.transfer_size;
	ctx->rx_buf = malloc(ctx->rx_size);
	if (!ctx->rx_buf)
	{
		fprintf(stderr, "Cannot allocate rx_buf\n");
		return -1;
	}

	ret = fi_mr_reg(ctx->domain, ctx->rx_buf, ctx->rx_size, FI_RECV, 0, FT_MR_KEY + 1, 0,
					&ctx->mr_multi_recv, NULL);
	if (ret)
	{
		FT_PRINTERR("fi_mr_reg", ret);
		return ret;
	}

	return 0;
}

int repost_multi_recv(struct ConnectionContext *ctx, int chunk)
{
	void *buf_addr;
	int ret;

	buf_addr = ctx->rx_buf + (ctx->rx_size / 2) * chunk;
	ret = fi_recv(ctx->ep, buf_addr, ctx->rx_size / 2,
				  fi_mr_desc(ctx->mr_multi_recv), 0,
				  &ctx->ctx_multi_recv[chunk]);
	if (ret)
	{
		FT_PRINTERR("fi_recv", ret);
		return ret;
	}

	return 0;
}

int wait_for_multi_recv_completion(struct ConnectionContext *ctx, int num_completions)
{
	int i, ret, per_buf_cnt = 0;
	struct fi_cq_data_entry comp;

	while (num_completions > 0) {
		ret = fi_cq_sread(ctx->rxcq, &comp, 64, NULL, ctx->timeout);
		if (ret == -FI_EAGAIN)
			continue;

		if (ret < 0) {
			FT_PRINTERR("fi_cq_read", ret);
			return ret;
		}

		if (comp.flags & FI_RECV) {
			if (comp.len != ctx->opts.transfer_size) {
				FT_ERR("completion length %lu, expected %lu",
					comp.len, ctx->opts.transfer_size);
				return -FI_EIO;
			}
			if (ft_check_opts(ctx, FT_OPT_VERIFY_DATA | FT_OPT_ACTIVE) &&
			    ft_check_buf(ctx, comp.buf, ctx->opts.transfer_size))
				return -FI_EIO;
			per_buf_cnt++;
			num_completions--;
		}

		if (comp.flags & FI_MULTI_RECV) {
			if (per_buf_cnt != ctx->comp_per_buf) {
				FT_ERR("Received %d completions per buffer, expected %d",
					per_buf_cnt, ctx->comp_per_buf);
				return -FI_EIO;
			}
			per_buf_cnt = 0;
			i = comp.op_context == &ctx->ctx_multi_recv[1];

			ret = repost_multi_recv(ctx, i);
			if (ret)
				return ret;
		}
	}
	return 0;
}
