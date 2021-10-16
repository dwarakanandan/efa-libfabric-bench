#include "common.h"

using namespace std;

void DEBUG(std::string str)
{
	if (FLAGS_debug)
	{
		std::cout << str << std::endl;
	}
}

void print_long_info(struct fi_info *info)
{
	struct fi_info *cur;
	for (cur = info; cur; cur = cur->next)
	{
		printf("\n\n---\n");
		printf("%s", fi_tostr(cur, FI_TYPE_INFO));
	}
}

void print_short_info(struct fi_info *info)
{
	struct fi_info *cur;
	for (cur = info; cur; cur = cur->next)
	{
		printf("provider: %s\n", cur->fabric_attr->prov_name);
		printf("    fabric: %s\n", cur->fabric_attr->name),
			printf("    domain: %s\n", cur->domain_attr->name),
			printf("    version: %d.%d\n", FI_MAJOR(cur->fabric_attr->prov_version),
				   FI_MINOR(cur->fabric_attr->prov_version));
		printf("    type: %s\n", fi_tostr(&cur->ep_attr->type, FI_TYPE_EP_TYPE));
		printf("    protocol: %s\n", fi_tostr(&cur->ep_attr->protocol, FI_TYPE_PROTOCOL));
	}
}

void generate_hints(struct fi_info **info)
{
	(*info)->fabric_attr->prov_name = const_cast<char *>(FLAGS_provider.c_str());
	(*info)->ep_attr->type = FI_EP_DGRAM;
	(*info)->mode = FI_MSG_PREFIX;
	(*info)->domain_attr->mode = ~0;
	(*info)->domain_attr->mr_mode = FI_MR_LOCAL | FI_MR_VIRT_ADDR | FI_MR_ALLOCATED | FI_MR_PROV_KEY;
}

int fabric_getinfo(struct ctx_connection *ct, struct fi_info *hints, struct fi_info **info)
{
	int ret;
	uint64_t flags = 0;

	ret = fi_getinfo(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION),
					 NULL, NULL, flags, hints, info);

	if (ret)
	{
		PRINTERR("fi_getinfo", ret);
		return ret;
	}

	ct->tx_ctx_ptr = NULL;
	ct->rx_ctx_ptr = NULL;

	print_short_info(*info);

	std::cout << std::endl
			  << std::endl;

	return 0;
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