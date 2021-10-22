#include "fi_pingpong_util.h"

using namespace std;

void DEBUG(std::string str)
{
	if (true)
	{
		std::cout << str << std::endl;
	}
}

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

uint64_t pp_gettime_us(void)
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
		printf("ctrl/read: no data or remote connection closed\n");
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
		printf("ctrl/read: no data or remote connection closed\n");
		return err;
	}

	return ret;
}

int pp_ctrl_recv_str(struct ctx_connection *ct, char *buf, size_t size)
{
	int ret;

	ret = pp_ctrl_recv(ct, buf, size);
	buf[size - 1] = '\0';
	return ret;
}

int open_fabric_res(struct ctx_connection *ct)
{
	int ret;

	DEBUG("Opening fabric resources: fabric, eq & domain\n");

	ret = fi_fabric(ct->fi->fabric_attr, &(ct->fabric), NULL);
	if (ret)
	{
		PRINTERR("fi_fabric", ret);
		return ret;
	}

	ret = fi_eq_open(ct->fabric, &(ct->eq_attr), &(ct->eq), NULL);
	if (ret)
	{
		PRINTERR("fi_eq_open", ret);
		return ret;
	}

	ret = fi_domain(ct->fabric, ct->fi, &(ct->domain), NULL);
	if (ret)
	{
		PRINTERR("fi_domain", ret);
		return ret;
	}

	DEBUG("Fabric resources opened\n");

	return 0;
}

int alloc_active_res(struct ctx_connection *ct, struct fi_info *fi)
{
	int ret;

	if (fi->tx_attr->mode & FI_MSG_PREFIX)
		ct->tx_prefix_size = fi->ep_attr->msg_prefix_size;
	if (fi->rx_attr->mode & FI_MSG_PREFIX)
		ct->rx_prefix_size = fi->ep_attr->msg_prefix_size;

	ret = pp_alloc_msgs(ct);
	if (ret)
		return ret;

	if (ct->cq_attr.format == FI_CQ_FORMAT_UNSPEC)
		ct->cq_attr.format = FI_CQ_FORMAT_CONTEXT;

	ct->cq_attr.wait_obj = FI_WAIT_NONE;

	ct->cq_attr.size = fi->tx_attr->size;
	ret = fi_cq_open(ct->domain, &(ct->cq_attr), &(ct->txcq), &(ct->txcq));
	if (ret)
	{
		PRINTERR("fi_cq_open", ret);
		return ret;
	}

	ct->cq_attr.size = fi->rx_attr->size;
	ret = fi_cq_open(ct->domain, &(ct->cq_attr), &(ct->rxcq), &(ct->rxcq));
	if (ret)
	{
		PRINTERR("fi_cq_open", ret);
		return ret;
	}

	if (fi->ep_attr->type == FI_EP_RDM ||
		fi->ep_attr->type == FI_EP_DGRAM)
	{
		if (fi->domain_attr->av_type != FI_AV_UNSPEC)
			ct->av_attr.type = fi->domain_attr->av_type;

		ret = fi_av_open(ct->domain, &(ct->av_attr), &(ct->av), NULL);
		if (ret)
		{
			PRINTERR("fi_av_open", ret);
			return ret;
		}
	}

	ret = fi_endpoint(ct->domain, fi, &(ct->ep), NULL);
	if (ret)
	{
		PRINTERR("fi_endpoint", ret);
		return ret;
	}

	return 0;
}

int init_ep(struct ctx_connection *ct)
{
	int ret;

	DEBUG("Initializing endpoint\n");

	if (ct->fi->ep_attr->type == FI_EP_MSG)
		PP_EP_BIND(ct->ep, ct->eq, 0);
	PP_EP_BIND(ct->ep, ct->av, 0);
	PP_EP_BIND(ct->ep, ct->txcq, FI_TRANSMIT);
	PP_EP_BIND(ct->ep, ct->rxcq, FI_RECV);

	ret = fi_enable(ct->ep);
	if (ret)
	{
		PRINTERR("fi_enable", ret);
		return ret;
	}

	ret = pp_post_rx(ct, ct->ep, MAX(ct->rx_size, PP_MAX_CTRL_MSG) + ct->rx_prefix_size, ct->rx_ctx_ptr);
	if (ret)
		return ret;

	DEBUG("Endpoint initialized\n");

	return 0;
}

int send_name(struct ctx_connection *ct, struct fid *endpoint)
{
	size_t addrlen = 0;
	uint32_t len;
	int ret;

	DEBUG("Fetching local address\n");

	ct->local_name = NULL;

	ret = fi_getname(endpoint, ct->local_name, &addrlen);
	if ((ret != -FI_ETOOSMALL) || (addrlen <= 0))
	{
		printf("fi_getname didn't return length\n");
		return -EMSGSIZE;
	}

	ct->local_name = (char *)calloc(1, addrlen);
	if (!ct->local_name)
	{
		printf("Failed to allocate memory for the address\n");
		return -ENOMEM;
	}

	ret = fi_getname(endpoint, ct->local_name, &addrlen);
	if (ret)
	{
		PRINTERR("fi_getname", ret);
		goto fn;
	}

	DEBUG("Sending name length\n");
	len = htonl(addrlen);
	ret = pp_ctrl_send(ct, (char *)&len, sizeof(len));
	if (ret < 0)
		goto fn;

	DEBUG("Sending address format\n");
	if (ct->fi)
	{
		ret = pp_ctrl_send(ct, (char *)&ct->fi->addr_format,
						   sizeof(ct->fi->addr_format));
	}
	else
	{
		ret = pp_ctrl_send(ct, (char *)&ct->fi_pep->addr_format,
						   sizeof(ct->fi_pep->addr_format));
	}
	if (ret < 0)
		goto fn;

	DEBUG("Sending name\n");
	ret = pp_ctrl_send(ct, ct->local_name, addrlen);
	DEBUG("Sent name\n");

fn:
	return ret;
}

int recv_name(struct ctx_connection *ct)
{
	uint32_t len;
	int ret;

	DEBUG("Receiving name length\n");
	ret = pp_ctrl_recv(ct, (char *)&len, sizeof(len));
	if (ret < 0)
		return ret;

	len = ntohl(len);
	if (len > PP_MAX_ADDRLEN)
		return -EINVAL;

	ct->rem_name = (char *)calloc(1, len);
	if (!ct->rem_name)
	{
		printf("Failed to allocate memory for the address\n");
		return -ENOMEM;
	}

	DEBUG("Receiving address format\n");
	ret = pp_ctrl_recv(ct, (char *)&ct->hints->addr_format,
					   sizeof(ct->hints->addr_format));
	if (ret < 0)
		return ret;

	DEBUG("Receiving name\n");
	ret = pp_ctrl_recv(ct, ct->rem_name, len);
	if (ret < 0)
		return ret;
	DEBUG("Received name\n");

	ct->hints->dest_addr = calloc(1, len);
	if (!ct->hints->dest_addr)
	{
		DEBUG("Failed to allocate memory for destination address\n");
		return -ENOMEM;
	}

	/* fi_freeinfo will free the dest_addr field. */
	memcpy(ct->hints->dest_addr, ct->rem_name, len);
	ct->hints->dest_addrlen = len;

	return 0;
}

int av_insert(struct fid_av *av, void *addr, size_t count, fi_addr_t *fi_addr, uint64_t flags, void *context)
{
	int ret;

	DEBUG("Connection-less endpoint: inserting new address in vector\n");

	ret = fi_av_insert(av, addr, count, fi_addr, flags, context);
	if (ret < 0)
	{
		PRINTERR("fi_av_insert", ret);
		return ret;
	}
	else if (ret != count)
	{
		printf("fi_av_insert: number of addresses inserted = %d;"
			   " number of addresses given = %zd\n",
			   ret, count);
		return -EXIT_FAILURE;
	}

	DEBUG("Connection-less endpoint: new address inserted in vector\n");

	return 0;
}

int pp_get_rx_comp(struct ctx_connection *ct, uint64_t total)
{
	int ret = FI_SUCCESS;

	if (ct->rxcq)
	{
		ret = pp_get_cq_comp(ct->rxcq, &(ct->rx_cq_cntr), total, ct->timeout_sec);
	}
	else
	{
		printf("Trying to get a RX completion when no RX CQ was opened");
		ret = -FI_EOTHER;
	}
	return ret;
}

static int pp_check_buf(void *buf, int size)
{
	char *recv_data;
	char c;
	static unsigned int iter;
	int msg_index;
	int i;

	// DEBUG("Verifying buffer content\n");

	msg_index = ((iter++) * INTEG_SEED) % integ_alphabet_length;
	recv_data = (char *)buf;

	for (i = 0; i < size; i++)
	{
		c = integ_alphabet[msg_index++];
		if (msg_index >= integ_alphabet_length)
			msg_index = 0;
		if (c != recv_data[i])
		{
			printf("index=%d msg_index=%d expected=%d got=%d\n",
				   i, msg_index, c, recv_data[i]);
			break;
		}
	}
	if (i != size)
	{
		DEBUG("Finished veryfing buffer: content is corrupted\n");
		printf("Error at iteration=%d size=%d byte=%d\n", iter, size,
			   i);
		return 1;
	}

	// DEBUG("Buffer verified\n");

	return 0;
}

ssize_t pp_rx(struct ctx_connection *ct, struct fid_ep *ep, size_t size)
{
	ssize_t ret;

	ret = pp_get_rx_comp(ct, ct->rx_seq);
	if (ret)
		return ret;

	// Verify data for now, disable during benchmarks
	if (false)
	{
		ret = pp_check_buf((char *)ct->rx_buf + ct->rx_prefix_size, size);
		if (ret)
			return ret;
	}

	ret = pp_post_rx(ct, ct->ep, MAX(ct->rx_size, PP_MAX_CTRL_MSG) + ct->rx_prefix_size, ct->rx_ctx_ptr);
	if (!ret)
		ct->cnt_ack_msg++;

	return ret;
}

static void pp_fill_buf(void *buf, int size)
{
	char *msg_buf;
	int msg_index;
	static unsigned int iter;
	int i;

	msg_index = ((iter++) * INTEG_SEED) % integ_alphabet_length;
	msg_buf = (char *)buf;
	for (i = 0; i < size; i++)
	{
		// printf("index=%d msg_index=%d\n", i, msg_index);
		msg_buf[i] = integ_alphabet[msg_index++];
		if (msg_index >= integ_alphabet_length)
			msg_index = 0;
	}
}
static int pp_get_tx_comp(struct ctx_connection *ct, uint64_t total)
{
	int ret;

	if (ct->txcq)
	{
		ret = pp_get_cq_comp(ct->txcq, &(ct->tx_cq_cntr), total, -1);
	}
	else
	{
		printf("Trying to get a TX completion when no TX CQ was opened");
		ret = -FI_EOTHER;
	}
	return ret;
}

static ssize_t pp_post_inject(struct ctx_connection *ct, struct fid_ep *ep, size_t size)
{
	if (!(ct->fi->caps & FI_TAGGED))
		PP_POST(fi_inject, pp_get_tx_comp, ct->tx_seq, "inject", ep,
				ct->tx_buf, size, ct->remote_fi_addr);
	else
		PP_POST(fi_tinject, pp_get_tx_comp, ct->tx_seq, "tinject", ep,
				ct->tx_buf, size, ct->remote_fi_addr, TAG);
	ct->tx_cq_cntr++;
	return 0;
}

ssize_t pp_inject(struct ctx_connection *ct, struct fid_ep *ep, size_t size)
{
	ssize_t ret;

	pp_fill_buf((char *)ct->tx_buf + ct->tx_prefix_size, size);

	ret = pp_post_inject(ct, ep, size + ct->tx_prefix_size);
	if (ret)
		return ret;

	return ret;
}

static ssize_t pp_post_tx(struct ctx_connection *ct, struct fid_ep *ep, size_t size, void *ctx)
{
	if (!(ct->fi->caps & FI_TAGGED))
		PP_POST(fi_send, pp_get_tx_comp, ct->tx_seq, "transmit", ep,
				ct->tx_buf, size, fi_mr_desc(ct->mr),
				ct->remote_fi_addr, ctx);
	else
		PP_POST(fi_tsend, pp_get_tx_comp, ct->tx_seq, "t-transmit", ep,
				ct->tx_buf, size, fi_mr_desc(ct->mr),
				ct->remote_fi_addr, TAG, ctx);
	return 0;
}

ssize_t pp_tx(struct ctx_connection *ct, struct fid_ep *ep, size_t size)
{
	ssize_t ret;

	pp_fill_buf((char *)ct->tx_buf + ct->tx_prefix_size, size);

	ret = pp_post_tx(ct, ep, size + ct->tx_prefix_size, ct->tx_ctx_ptr);
	if (ret)
		return ret;

	ret = pp_get_tx_comp(ct, ct->tx_seq);

	return ret;
}

char *size_str(char *str, uint64_t size)
{
	uint64_t base, fraction = 0;
	char mag;

	memset(str, '\0', PP_STR_LEN);

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
		snprintf(str, PP_STR_LEN, "%" PRIu64 ".%" PRIu64 "%c",
				 size / base, fraction, mag);
	else
		snprintf(str, PP_STR_LEN, "%" PRIu64 "%c", size / base, mag);

	return str;
}

char *cnt_str(char *str, size_t size, uint64_t cnt)
{
	if (cnt >= 1000000000)
		snprintf(str, size, "%" PRIu64 "b", cnt / 1000000000);
	else if (cnt >= 1000000)
		snprintf(str, size, "%" PRIu64 "m", cnt / 1000000);
	else if (cnt >= 1000)
		snprintf(str, size, "%" PRIu64 "k", cnt / 1000);
	else
		snprintf(str, size, "%" PRIu64, cnt);

	return str;
}

long parse_ulong(char *str, long max)
{
	long ret;
	char *end;

	errno = 0;
	ret = strtol(str, &end, 10);
	if (*end != '\0' || errno != 0)
	{
		if (errno == 0)
			ret = -EINVAL;
		else
			ret = -errno;
		fprintf(stderr, "Error parsing \"%s\": %s\n", str,
				strerror(-ret));
		return ret;
	}

	if ((ret < 0) || (max > 0 && ret > max))
	{
		ret = -ERANGE;
		fprintf(stderr, "Error parsing \"%s\": %s\n", str,
				strerror(-ret));
		return ret;
	}
	return ret;
}

void show_perf(char *name, int tsize, int sent, int acked, uint64_t start, uint64_t end, int xfers_per_iter)
{
	static int header = 1;
	char str[PP_STR_LEN];
	int64_t elapsed = end - start;
	uint64_t bytes = (uint64_t)sent * tsize * xfers_per_iter;
	float usec_per_xfer;

	if (sent == 0)
		return;

	if (name)
	{
		if (header)
		{
			printf("%-50s%-8s%-8s%-9s%-8s%8s %10s%13s%13s\n",
				   "name", "bytes", "#sent", "#ack", "total",
				   "time", "MB/sec", "usec/xfer", "Mxfers/sec");
			header = 0;
		}

		printf("%-50s", name);
	}
	else
	{
		if (header)
		{
			printf("%-8s%-8s%-9s%-8s%8s %10s%13s%13s\n", "bytes",
				   "#sent", "#ack", "total", "time", "MB/sec",
				   "usec/xfer", "Mxfers/sec");
			header = 0;
		}
	}

	printf("%-8s", size_str(str, tsize));
	printf("%-8s", cnt_str(str, sizeof(str), sent));

	if (sent == acked)
		printf("=%-8s", cnt_str(str, sizeof(str), acked));
	else if (sent < acked)
		printf("-%-8s", cnt_str(str, sizeof(str), acked - sent));
	else
		printf("+%-8s", cnt_str(str, sizeof(str), sent - acked));

	printf("%-8s", size_str(str, bytes));

	usec_per_xfer = ((float)elapsed / sent / xfers_per_iter);
	printf("%8.2fs%10.2f%11.2f%11.2f\n", elapsed / 1000000.0,
		   bytes / (1.0 * elapsed), usec_per_xfer, 1.0 / usec_per_xfer);
}
