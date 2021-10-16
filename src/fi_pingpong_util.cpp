#include "fi_pingpong_util.h"

uint64_t init_cq_data(struct fi_info *info)
{
	if (info->domain_attr->cq_data_size >= sizeof(uint64_t))
	{
		return 0x0123456789abcdefULL;
	}
	else
	{
		return 0x0123456789abcdefULL &
			   ((0x1ULL << (info->domain_attr->cq_data_size * 8)) - 1);
	}
}

int pp_alloc_msgs(struct ctx_connection *ct)
{
	int ret;
	long alignment = 1;

	ct->tx_size = PP_MAX_DATA_MSG;
	if (ct->tx_size > ct->fi->ep_attr->max_msg_size)
		ct->tx_size = ct->fi->ep_attr->max_msg_size;
	ct->rx_size = ct->tx_size;
	ct->buf_size = MAX(ct->tx_size, PP_MAX_CTRL_MSG) +
				   MAX(ct->rx_size, PP_MAX_CTRL_MSG) +
				   ct->tx_prefix_size + ct->rx_prefix_size;

	alignment = 4096;
	if (alignment < 0)
	{
		PRINTERR("ofi_get_page_size", alignment);
		return alignment;
	}
	/* Extra alignment for the second part of the buffer */
	ct->buf_size += alignment;

	ret = posix_memalign(&(ct->buf), (size_t)alignment, ct->buf_size);
	if (ret)
	{
		PRINTERR("ofi_memalign", ret);
		return ret;
	}
	memset(ct->buf, 0, ct->buf_size);
	ct->rx_buf = ct->buf;
	ct->tx_buf = (char *)ct->buf +
				 MAX(ct->rx_size, PP_MAX_CTRL_MSG) +
				 ct->tx_prefix_size;
	ct->tx_buf = (void *)(((uintptr_t)ct->tx_buf + alignment - 1) &
						  ~(alignment - 1));

	ct->remote_cq_data = init_cq_data(ct->fi);

	if (ct->fi->domain_attr->mr_mode & FI_MR_LOCAL)
	{
		ret = fi_mr_reg(ct->domain, ct->buf, ct->buf_size,
						FI_SEND | FI_RECV, 0, PP_MR_KEY, 0, &(ct->mr),
						NULL);
		if (ret)
		{
			PRINTERR("fi_mr_reg", ret);
			return ret;
		}
	}
	else
	{
		ct->mr = &(ct->no_mr);
	}

	return 0;
}

static uint64_t pp_gettime_us(void)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	return now.tv_sec * 1000000 + now.tv_usec;
}

static int pp_cq_readerr(struct fid_cq *cq)
{
	struct fi_cq_err_entry cq_err = {0};
	int ret;

	ret = fi_cq_readerr(cq, &cq_err, 0);
	if (ret < 0)
	{
		PRINTERR("fi_cq_readerr", ret);
	}
	else
	{
		printf("cq_readerr: %s",
			   fi_cq_strerror(cq, cq_err.prov_errno, cq_err.err_data,
							  NULL, 0));
		ret = -cq_err.err;
	}
	return ret;
}

int pp_get_cq_comp(struct fid_cq *cq, uint64_t *cur, uint64_t total, int timeout_sec)
{
	struct fi_cq_err_entry comp;
	uint64_t a = 0, b = 0;
	int ret = 0;

	if (timeout_sec >= 0)
		a = pp_gettime_us();

	do
	{
		ret = fi_cq_read(cq, &comp, 1);
		if (ret > 0)
		{
			if (timeout_sec >= 0)
				a = pp_gettime_us();

			(*cur)++;
		}
		else if (ret < 0 && ret != -FI_EAGAIN)
		{
			if (ret == -FI_EAVAIL)
			{
				ret = pp_cq_readerr(cq);
				(*cur)++;
			}
			else
			{
				PRINTERR("pp_get_cq_comp", ret);
			}

			return ret;
		}
		else if (timeout_sec >= 0)
		{
			b = pp_gettime_us();
			if ((b - a) / 1000000 > timeout_sec)
			{
				fprintf(stderr, "%ds timeout expired\n",
						timeout_sec);
				return -FI_ENODATA;
			}
		}
	} while (total - *cur > 0);

	return 0;
}

int pp_get_rx_comp(struct ctx_connection *ct, uint64_t total)
{
	int ret = FI_SUCCESS;

	if (ct->rxcq)
	{
		ret = pp_get_cq_comp(ct->rxcq, &(ct->rx_cq_cntr), total,
							 ct->timeout_sec);
	}
	else
	{
		printf(
			"Trying to get a RX completion when no RX CQ was opened");
		ret = -FI_EOTHER;
	}
	return ret;
}

ssize_t pp_post_rx(struct ctx_connection *ct, struct fid_ep *ep, size_t size, void *ctx)
{
	if (!(ct->fi->caps & FI_TAGGED))
		PP_POST(fi_recv, pp_get_rx_comp, ct->rx_seq, "receive", ep,
				ct->rx_buf, size, fi_mr_desc(ct->mr), 0, ctx);
	else
		PP_POST(fi_trecv, pp_get_rx_comp, ct->rx_seq, "t-receive", ep,
				ct->rx_buf, size, fi_mr_desc(ct->mr), 0, TAG, 0, ctx);
	return 0;
}

int pp_ctrl_send(struct ctx_connection *ct, char *buf, size_t size)
{
	int ret, err;

	ret = send(ct->ctrl_connfd, buf, size, 0);
	if (ret < 0)
	{
		err = -1;
		PRINTERR("ctrl/send", err);
		return err;
	}
	if (ret == 0)
	{
		err = -ECONNABORTED;
		printf("ctrl/read: no data or remote connection closed");
		return err;
	}

	return ret;
}

int pp_ctrl_recv(struct ctx_connection *ct, char *buf, size_t size)
{
	int ret, err;

	do
	{
		ret = read(ct->ctrl_connfd, buf, size);
	} while (ret == -1);
	if (ret < 0)
	{
		err = -1;
		PRINTERR("ctrl/read", err);
		return err;
	}
	if (ret == 0)
	{
		err = -ECONNABORTED;
		printf("ctrl/read: no data or remote connection closed");
		return err;
	}

	return ret;
}