#include "FabricUtil.h"

int libefa::FabricUtil::openFabricRes(ConnectionContext *ctx)
{
	int ret;

	DEBUG("Opening fabric resources: fabric, eq & domain\n");

	ret = fi_fabric(ctx->fi.info->fabric_attr, &(ctx->fabric), NULL);
	if (ret)
	{
		PRINTERR("fi_fabric", ret);
		return ret;
	}

	ret = fi_eq_open(ctx->fabric, &(ctx->eq_attr), &(ctx->eq), NULL);
	if (ret)
	{
		PRINTERR("fi_eq_open", ret);
		return ret;
	}

	ret = fi_domain(ctx->fabric, ctx->fi.info, &(ctx->domain), NULL);

	if (ret)
	{
		PRINTERR("fi_domain", ret);
		return ret;
	}

	DEBUG("Fabric resources opened\n");

	return 0;
}

uint64_t libefa::FabricUtil::getTimeMicroSeconds()
{
	struct timeval now;

	gettimeofday(&now, NULL);
	return now.tv_sec * 1000000 + now.tv_usec;
}

uint64_t libefa::FabricUtil::initCqData(struct fi_info *info)
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

int libefa::FabricUtil::allocMessages(ConnectionContext *ctx)
{
	int ret;
	long alignment = 1;

	ctx->tx_size = (ctx->fi.hints->ep_attr->type == FI_EP_RDM) ? MAX_DATA_MSG_RDM : MAX_DATA_MSG_DGRAM;
	if (ctx->tx_size > ctx->fi.info->ep_attr->max_msg_size)
		ctx->tx_size = ctx->fi.info->ep_attr->max_msg_size;
	ctx->rx_size = ctx->tx_size;
	ctx->buf_size = MAX(ctx->tx_size, MAX_CTRL_MSG) +
					MAX(ctx->rx_size, MAX_CTRL_MSG) +
					ctx->tx_prefix_size + ctx->rx_prefix_size;

	alignment = 4096;
	if (alignment < 0)
	{
		PRINTERR("ofi_get_page_size", alignment);
		return alignment;
	}
	/* Extra alignment for the second part of the buffer */
	ctx->buf_size += alignment;

	ret = posix_memalign(&(ctx->buf), (size_t)alignment, ctx->buf_size);
	if (ret)
	{
		PRINTERR("ofi_memalign", ret);
		return ret;
	}
	memset(ctx->buf, 0, ctx->buf_size);
	ctx->rx_buf = ctx->buf;
	ctx->tx_buf = (char *)ctx->buf +
				  MAX(ctx->rx_size, MAX_CTRL_MSG) +
				  ctx->tx_prefix_size;
	ctx->tx_buf = (void *)(((uintptr_t)ctx->tx_buf + alignment - 1) &
						   ~(alignment - 1));

	ctx->remote_cq_data = initCqData(ctx->fi.info);

	// if (ctx->fi.info->domain_attr->mr_mode & FI_MR_LOCAL)
	if(true)
	{
		ret = fi_mr_reg(ctx->domain, ctx->buf, ctx->buf_size,
						FI_SEND | FI_RECV | FI_READ | FI_WRITE, 0, PP_MR_KEY, 0, &(ctx->mr),
						NULL);
		if (ret)
		{
			PRINTERR("fi_mr_reg", ret);
			return ret;
		}
	}
	else
	{
		ctx->mr = &(ctx->no_mr);
	}
	return 0;
}

int libefa::FabricUtil::allocActiveRes(ConnectionContext *ctx)
{
	int ret;

	if (ctx->fi.info->tx_attr->mode & FI_MSG_PREFIX)
		ctx->tx_prefix_size = ctx->fi.info->ep_attr->msg_prefix_size;
	if (ctx->fi.info->rx_attr->mode & FI_MSG_PREFIX)
		ctx->rx_prefix_size = ctx->fi.info->ep_attr->msg_prefix_size;

	ret = FabricUtil::allocMessages(ctx);
	if (ret)
		return ret;

	if (ctx->cq_attr.format == FI_CQ_FORMAT_UNSPEC)
		ctx->cq_attr.format = FI_CQ_FORMAT_CONTEXT;

	ctx->cq_attr.wait_obj = FI_WAIT_NONE;

	ctx->cq_attr.size = ctx->fi.info->tx_attr->size;
	ret = fi_cq_open(ctx->domain, &(ctx->cq_attr), &(ctx->txcq), &(ctx->txcq));
	if (ret)
	{
		PRINTERR("fi_cq_open", ret);
		return ret;
	}

	ctx->cq_attr.size = ctx->fi.info->rx_attr->size;
	ret = fi_cq_open(ctx->domain, &(ctx->cq_attr), &(ctx->rxcq), &(ctx->rxcq));
	if (ret)
	{
		PRINTERR("fi_cq_open", ret);
		return ret;
	}

	if (ctx->fi.info->ep_attr->type == FI_EP_RDM ||
		ctx->fi.info->ep_attr->type == FI_EP_DGRAM)
	{
		if (ctx->fi.info->domain_attr->av_type != FI_AV_UNSPEC)
			ctx->av_attr.type = ctx->fi.info->domain_attr->av_type;

		ret = fi_av_open(ctx->domain, &(ctx->av_attr), &(ctx->av), NULL);
		if (ret)
		{
			PRINTERR("fi_av_open", ret);
			return ret;
		}
	}

	ret = fi_endpoint(ctx->domain, ctx->fi.info, &(ctx->ep), NULL);
	if (ret)
	{
		PRINTERR("fi_endpoint", ret);
		return ret;
	}

	return 0;
}

int libefa::FabricUtil::initEndpoint(ConnectionContext *ctx)
{
	int ret;

	DEBUG("Initializing endpoint\n");

	if (ctx->fi.info->ep_attr->type == FI_EP_MSG)
		EP_BIND(ctx->ep, ctx->eq, 0);
	EP_BIND(ctx->ep, ctx->av, 0);
	EP_BIND(ctx->ep, ctx->txcq, FI_TRANSMIT);
	EP_BIND(ctx->ep, ctx->rxcq, FI_RECV);

	ret = fi_enable(ctx->ep);
	if (ret)
	{
		PRINTERR("fi_enable", ret);
		return ret;
	}

	ret = FabricUtil::postRx(ctx, ctx->ep, MAX(ctx->rx_size, MAX_CTRL_MSG) + ctx->rx_prefix_size, ctx->rx_ctx_ptr);
	if (ret)
		return ret;

	DEBUG("Endpoint initialized\n");

	return 0;
}

int libefa::FabricUtil::getRxCompletion(ConnectionContext *ctx, uint64_t total)
{
	int ret = FI_SUCCESS;

	if (ctx->rxcq)
	{
		ret = getCqCompletion(ctx->rxcq, &(ctx->rx_cq_cntr), total, ctx->timeout_sec);
	}
	else
	{
		printf("Trying to get a RX completion when no RX CQ was opened");
		ret = -FI_EOTHER;
	}
	return ret;
}

ssize_t libefa::FabricUtil::postRx(ConnectionContext *ctx, struct fid_ep *ep, size_t size, void *ctxptr)
{
	if (!(ctx->fi.info->caps & FI_TAGGED))
	{
		do
		{
			int timeout_sec_save;
			int ret, rc;

			while (1)
			{
				ret = fi_recv(ep, ctx->rx_buf, size, fi_mr_desc(ctx->mr), 0, ctxptr);
				if (!ret)
					break;

				if (ret != -FI_EAGAIN)
				{
					PRINTERR("receive", ret);
					return ret;
				}

				timeout_sec_save = ctx->timeout_sec;
				ctx->timeout_sec = 0;
				rc = FabricUtil::getRxCompletion(ctx, ctx->rx_seq);
				ctx->timeout_sec = timeout_sec_save;
				if (rc && rc != -FI_EAGAIN)
				{
					printf("Failed to get receive completion");
					return rc;
				}
			}
			ctx->rx_seq++;
		} while (0);
	}
	else
	{
		do
		{
			int timeout_sec_save;
			int ret, rc;

			while (1)
			{
				ret = fi_trecv(ep, ctx->rx_buf, size, fi_mr_desc(ctx->mr), 0, TAG, 0, ctxptr);
				if (!ret)
					break;

				if (ret != -FI_EAGAIN)
				{
					PRINTERR("t-receive", ret);
					return ret;
				}

				timeout_sec_save = ctx->timeout_sec;
				ctx->timeout_sec = 0;
				rc = FabricUtil::getRxCompletion(ctx, ctx->rx_seq);
				ctx->timeout_sec = timeout_sec_save;
				if (rc && rc != -FI_EAGAIN)
				{
					printf("Failed to get t-receive completion");
					return rc;
				}
			}
			ctx->rx_seq++;
		} while (0);
	}
	return 0;
}

int libefa::FabricUtil::sendName(ConnectionContext *ctx, struct fid *endpoint)
{
	size_t addrlen = 0;
	uint32_t len;
	int ret;

	DEBUG("Fetching local address\n");

	ctx->local_name = NULL;

	ret = fi_getname(endpoint, ctx->local_name, &addrlen);
	if ((ret != -FI_ETOOSMALL) || (addrlen <= 0))
	{
		printf("fi_getname didn't return length\n");
		return -EMSGSIZE;
	}

	ctx->local_name = (char *)calloc(1, addrlen);
	if (!ctx->local_name)
	{
		printf("Failed to allocate memory for the address\n");
		return -ENOMEM;
	}

	ret = fi_getname(endpoint, ctx->local_name, &addrlen);
	if (ret)
	{
		PRINTERR("fi_getname", ret);
		goto fn;
	}

	DEBUG("Sending name length\n");
	len = htonl(addrlen);
	ret = FabricUtil::ctrlSend(ctx, (char *)&len, sizeof(len));
	if (ret < 0)
		goto fn;

	DEBUG("Sending address format\n");
	if (ctx->fi.info)
	{
		ret = FabricUtil::ctrlSend(ctx, (char *)&ctx->fi.info->addr_format,
								   sizeof(ctx->fi.info->addr_format));
	}
	else
	{
		ret = FabricUtil::ctrlSend(ctx, (char *)&ctx->fi_pep->addr_format,
								   sizeof(ctx->fi_pep->addr_format));
	}
	if (ret < 0)
		goto fn;

	DEBUG("Sending name\n");
	ret = FabricUtil::ctrlSend(ctx, ctx->local_name, addrlen);
	DEBUG("Sent name\n");

fn:
	return ret;
}

int libefa::FabricUtil::receiveName(ConnectionContext *ctx)
{
	uint32_t len;
	int ret;

	DEBUG("Receiving name length\n");
	ret = FabricUtil::ctrlReceive(ctx, (char *)&len, sizeof(len));
	if (ret < 0)
		return ret;

	len = ntohl(len);
	if (len > PP_MAX_ADDRLEN)
		return -EINVAL;

	ctx->rem_name = (char *)calloc(1, len);
	if (!ctx->rem_name)
	{
		printf("Failed to allocate memory for the address\n");
		return -ENOMEM;
	}

	DEBUG("Receiving address format\n");
	ret = FabricUtil::ctrlReceive(ctx, (char *)&ctx->fi.hints->addr_format,
								  sizeof(ctx->fi.hints->addr_format));
	if (ret < 0)
		return ret;

	DEBUG("Receiving name\n");
	ret = FabricUtil::ctrlReceive(ctx, ctx->rem_name, len);
	if (ret < 0)
		return ret;
	DEBUG("Received name\n");

	ctx->fi.hints->dest_addr = calloc(1, len);
	if (!ctx->fi.hints->dest_addr)
	{
		DEBUG("Failed to allocate memory for destination address\n");
		return -ENOMEM;
	}

	/* fi_freefi.info will free the dest_addr field. */
	memcpy(ctx->fi.hints->dest_addr, ctx->rem_name, len);
	ctx->fi.hints->dest_addrlen = len;

	return 0;
}

int libefa::FabricUtil::ctrlSend(ConnectionContext *ctx, char *buf, size_t size)
{
	int ret, err;

	ret = send(ctx->ctrl_connfd, buf, size, 0);
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

int libefa::FabricUtil::ctrlReceive(ConnectionContext *ctx, char *buf, size_t size)
{
	int ret, err;

	do
	{
		ret = read(ctx->ctrl_connfd, buf, size);
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

int libefa::FabricUtil::ctrlReceiveString(ConnectionContext *ctx, char *buf, size_t size)
{
	int ret;

	ret = FabricUtil::ctrlReceive(ctx, buf, size);
	buf[size - 1] = '\0';
	return ret;
}

int libefa::FabricUtil::insertAddressVector(struct fid_av *av, void *addr, size_t count, fi_addr_t *fi_addr, uint64_t flags, void *context)
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

void libefa::FabricUtil::prinAddrinfo(struct addrinfo *ai, std::string msg)
{
	char s[80] = {0};
	void *addr;

	addr = &((struct sockaddr_in *)ai->ai_addr)->sin_addr;

	inet_ntop(ai->ai_family, addr, s, 80);
	std::cout << msg << " " << s << std::endl;
}

ssize_t libefa::FabricUtil::rx(ConnectionContext *ctx, struct fid_ep *ep, size_t size)
{
	ssize_t ret;

	ret = getRxCompletion(ctx, ctx->rx_seq);
	if (ret)
		return ret;

	ret = postRx(ctx, ctx->ep, MAX(ctx->rx_size, MAX_CTRL_MSG) + ctx->rx_prefix_size, ctx->rx_ctx_ptr);
	if (!ret)
		ctx->cnt_ack_msg++;

	return ret;
}

void libefa::FabricUtil::fillBuffer(void *buf, int size)
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

int libefa::FabricUtil::cqReadError(struct fid_cq *cq)
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

int libefa::FabricUtil::getCqCompletion(struct fid_cq *cq, uint64_t *cur, uint64_t total, int timeout_sec)
{
	struct fi_cq_err_entry comp;
	uint64_t a = 0, b = 0;
	int ret = 0;

	if (timeout_sec >= 0)
		a = getTimeMicroSeconds();

	do
	{
		ret = fi_cq_read(cq, &comp, 1);
		if (ret > 0)
		{
			if (timeout_sec >= 0)
				a = getTimeMicroSeconds();

			(*cur)++;
		}
		else if (ret < 0 && ret != -FI_EAGAIN)
		{
			if (ret == -FI_EAVAIL)
			{
				ret = cqReadError(cq);
				(*cur)++;
			}
			else
			{
				PRINTERR("get_cq_comp", ret);
			}

			return ret;
		}
		else if (timeout_sec >= 0)
		{
			b = getTimeMicroSeconds();
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

int libefa::FabricUtil::getTxCompletion(ConnectionContext *ctx, uint64_t total)
{
	int ret;

	if (ctx->txcq)
	{
		ret = getCqCompletion(ctx->txcq, &(ctx->tx_cq_cntr), total, -1);
	}
	else
	{
		printf("Trying to get a TX completion when no TX CQ was opened");
		ret = -FI_EOTHER;
	}
	return ret;
}

ssize_t libefa::FabricUtil::postTx(ConnectionContext *ctx, struct fid_ep *ep, size_t size, void *ctxptr)
{
	if (!(ctx->fi.info->caps & FI_TAGGED))
	{
		do
		{
			int timeout_sec_save;
			int ret, rc;

			while (1)
			{
				ret = fi_send(ep, ctx->tx_buf, size, fi_mr_desc(ctx->mr), ctx->remote_fi_addr, ctxptr);
				if (!ret)
					break;

				if (ret != -FI_EAGAIN)
				{
					PRINTERR("transmit", ret);
					return ret;
				}

				timeout_sec_save = ctx->timeout_sec;
				ctx->timeout_sec = 0;
				rc = FabricUtil::getTxCompletion(ctx, ctx->tx_seq);
				ctx->timeout_sec = timeout_sec_save;
				if (rc && rc != -FI_EAGAIN)
				{
					printf("Failed to get transmit completion");
					return rc;
				}
			}
			ctx->tx_seq++;
		} while (0);
	}
	else
	{
		do
		{
			int timeout_sec_save;
			int ret, rc;

			while (1)
			{
				ret = fi_tsend(ep, ctx->tx_buf, size, fi_mr_desc(ctx->mr), ctx->remote_fi_addr, TAG, ctxptr);
				if (!ret)
					break;

				if (ret != -FI_EAGAIN)
				{
					PRINTERR("t-transmit", ret);
					return ret;
				}

				timeout_sec_save = ctx->timeout_sec;
				ctx->timeout_sec = 0;
				rc = FabricUtil::getTxCompletion(ctx, ctx->tx_seq);
				ctx->timeout_sec = timeout_sec_save;
				if (rc && rc != -FI_EAGAIN)
				{
					printf("Failed to get t-transmit completion");
					return rc;
				}
			}
			ctx->tx_seq++;
		} while (0);
	}
	return 0;
}

ssize_t libefa::FabricUtil::postInject(ConnectionContext *ctx, struct fid_ep *ep, size_t size, void *ctxptr)
{
	if (!(ctx->fi.info->caps & FI_TAGGED))
	{
		do
		{
			int timeout_sec_save;
			int ret, rc;

			while (1)
			{
				ret = fi_inject(ep, ctx->tx_buf, size, ctx->remote_fi_addr);
				if (!ret)
					break;

				if (ret != -FI_EAGAIN)
				{
					PRINTERR("transmit", ret);
					return ret;
				}

				timeout_sec_save = ctx->timeout_sec;
				ctx->timeout_sec = 0;
				rc = FabricUtil::getTxCompletion(ctx, ctx->tx_seq);
				ctx->timeout_sec = timeout_sec_save;
				if (rc && rc != -FI_EAGAIN)
				{
					printf("Failed to get transmit completion");
					return rc;
				}
			}
			ctx->tx_seq++;
		} while (0);
	}
	else
	{
		do
		{
			int timeout_sec_save;
			int ret, rc;

			while (1)
			{
				ret = fi_tinject(ep, ctx->tx_buf, size, ctx->remote_fi_addr, TAG);
				if (!ret)
					break;

				if (ret != -FI_EAGAIN)
				{
					PRINTERR("t-transmit", ret);
					return ret;
				}

				timeout_sec_save = ctx->timeout_sec;
				ctx->timeout_sec = 0;
				rc = FabricUtil::getTxCompletion(ctx, ctx->tx_seq);
				ctx->timeout_sec = timeout_sec_save;
				if (rc && rc != -FI_EAGAIN)
				{
					printf("Failed to get t-transmit completion");
					return rc;
				}
			}
			ctx->tx_seq++;
		} while (0);
	}
	return 0;
}

ssize_t libefa::FabricUtil::tx(ConnectionContext *ctx, struct fid_ep *ep, size_t size)
{
	ssize_t ret;

	fillBuffer((char *)ctx->tx_buf + ctx->tx_prefix_size, size);

	ret = postTx(ctx, ep, size + ctx->tx_prefix_size, ctx->tx_ctx_ptr);
	if (ret)
		return ret;

	ret = getTxCompletion(ctx, ctx->tx_seq);

	return ret;
}

ssize_t libefa::FabricUtil::inject(ConnectionContext *ctx, struct fid_ep *ep, size_t size)
{
	ssize_t ret;

	fillBuffer((char *)ctx->tx_buf + ctx->tx_prefix_size, size);

	ret = postInject(ctx, ep, size + ctx->tx_prefix_size, ctx->tx_ctx_ptr);

	return ret;
}

int libefa::FabricUtil::ctrlSendRmaIov(ConnectionContext *ctx)
{
	int ret;
    uint64_t addr = (uintptr_t)ctx->rx_buf + ctx->rx_prefix_size;
    std::string addr_str = std::to_string(addr);
    std::string addr_str_length_str = std::to_string(addr_str.length() + 1);

    snprintf(ctx->ctrl_buf, 3, "%s", addr_str_length_str.c_str());
    ret = FabricUtil::ctrlSend(ctx, ctx->ctrl_buf, 3);
    if (ret < 0)
        return ret;

    snprintf(ctx->ctrl_buf, addr_str.length() + 1, "%s", addr_str.c_str());
    ret = FabricUtil::ctrlSend(ctx, ctx->ctrl_buf, addr_str.length() + 1);
    if (ret < 0)
        return ret;

    uint64_t key = fi_mr_key(ctx->mr);
    std::string key_str = std::to_string(key);
    std::string key_str_length_str = std::to_string(key_str.length() + 1);

    snprintf(ctx->ctrl_buf, 3, "%s", key_str_length_str.c_str());
    ret = FabricUtil::ctrlSend(ctx, ctx->ctrl_buf, 3);
    if (ret < 0)
        return ret;

    snprintf(ctx->ctrl_buf, key_str.length() + 1, "%s", key_str.c_str());
    ret = FabricUtil::ctrlSend(ctx, ctx->ctrl_buf, key_str.length() + 1);
    if (ret < 0)
        return ret;

	return 0;
}

int libefa::FabricUtil::ctrlReceiveRmaIov(ConnectionContext *ctx)
{
	struct fi_rma_iov rma_iov;
    int ret;

    ret = ctrlReceive(ctx, ctx->ctrl_buf, 3);
    if (ret < 0)
        return ret;

    int addr_length = std::stoi(ctx->ctrl_buf);
    ret = FabricUtil::ctrlReceive(ctx, ctx->ctrl_buf, addr_length);
    if (ret < 0)
        return ret;

    rma_iov.addr = std::stoul(ctx->ctrl_buf);

    ret = FabricUtil::ctrlReceive(ctx, ctx->ctrl_buf, 3);
    if (ret < 0)
        return ret;

    int key_length = std::stoi(ctx->ctrl_buf);
    ret = FabricUtil::ctrlReceive(ctx, ctx->ctrl_buf, key_length);
    if (ret < 0)
        return ret;

    rma_iov.key = std::stoul(ctx->ctrl_buf);

	ctx->remote_rma_iov = &rma_iov;

	return 0;
}