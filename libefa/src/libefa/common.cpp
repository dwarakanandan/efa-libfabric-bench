#include "common.h"

using namespace std;

void generate_hints(struct fi_info **info)
{
	//(*info)->fabric_attr->prov_name = const_cast<char *>(FLAGS_provider.c_str());
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

	return 0;
}

void banner_fabric_info(struct ctx_connection *ct)
{
	struct fi_info *cur = ct->fi;
	printf("Running benchmark with fi_info:\n\n");
	printf("provider: %s\n", cur->fabric_attr->prov_name);
	printf("    fabric: %s\n", cur->fabric_attr->name),
		printf("    domain: %s\n", cur->domain_attr->name),
		printf("    version: %d.%d\n", FI_MAJOR(cur->fabric_attr->prov_version),
			   FI_MINOR(cur->fabric_attr->prov_version));
	printf("    type: %s\n", fi_tostr(&cur->ep_attr->type, FI_TYPE_EP_TYPE));
	printf("    protocol: %s\n", fi_tostr(&cur->ep_attr->protocol, FI_TYPE_PROTOCOL));
	std::cout << std::endl
			  << std::endl;
}

void chrono_start(struct ctx_connection *ct)
{
	DEBUG("Starting timekeeper\n");
	ct->start = pp_gettime_us();
}

void chrono_stop(struct ctx_connection *ct)
{
	ct->end = pp_gettime_us();
	DEBUG("Stopped timekeeper\n");
}


