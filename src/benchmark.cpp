#include<iostream>

#include <rdma/fabric.h>
#include <rdma/fi_errno.h>

using namespace std;

static int print_short_info(struct fi_info *info)
{
	struct fi_info *cur;
	for (cur = info; cur; cur = cur->next) {
		printf("provider: %s\n", cur->fabric_attr->prov_name);
		printf("    fabric: %s\n", cur->fabric_attr->name),
		printf("    domain: %s\n", cur->domain_attr->name),
		printf("    version: %d.%d\n", FI_MAJOR(cur->fabric_attr->prov_version),
			FI_MINOR(cur->fabric_attr->prov_version));
		printf("    type: %s\n", fi_tostr(&cur->ep_attr->type, FI_TYPE_EP_TYPE));
		printf("    protocol: %s\n", fi_tostr(&cur->ep_attr->protocol, FI_TYPE_PROTOCOL));
	}
	return EXIT_SUCCESS;
}

int main(int argc, char const *argv[])
{
    struct fi_info *hints, *info;

    hints = fi_allocinfo();

    fi_getinfo(FI_VERSION(1, 4), NULL, NULL, 0, hints, &info);
    print_short_info(info);
    fi_freeinfo(hints);
    return 0;
}
