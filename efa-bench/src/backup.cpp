#include <libefa/ftlib/shared.h>
#include <stdio.h>
#include <stdlib.h>
#include <libefa/Server.h>
#include <libefa/Client.h>


// int main(int argc, char **argv)
// {
// 	int op, ret;

// 	init_opts(&opts);
// 	opts.options |= FT_OPT_BW;

// 	hints = fi_allocinfo();
// 	if (!hints)
// 		return EXIT_FAILURE;

// 	hints->caps = FI_MSG | FI_RMA;
// 	hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
// 	hints->mode = FI_CONTEXT;
// 	hints->domain_attr->threading = FI_THREAD_DOMAIN;
// 	hints->addr_format = opts.address_format;
// 	hints->domain_attr->mr_mode = opts.mr_mode;
// 	hints->tx_attr->tclass = FI_TC_BULK_DATA;

// 	ft_parseinfo('p', strdup("efa"), hints, &opts);
// 	ft_parseinfo('e', strdup("rdm"), hints, &opts);
// 	ft_parsecsopts('b', NULL, &opts);
// 	ret = ft_parse_rma_opts('o', strdup("write"), hints, &opts);

// 	if (argc > 1)
// 		opts.dst_addr = strdup("127.0.0.1");

// 	ret = ft_init_fabric();
// 	if (ret)
// 		return ret;

// 	ret = ft_exchange_keys(&remote);
// 	if (ret)
// 		return ret;

// 	ret = ft_sync();
// 	if (ret)
// 		return ret;

// 	ret = ft_post_rma(opts.rma_op, ep, opts.transfer_size,
// 					  &remote, NULL);
// 	if (ret)
// 		return ret;

// 	printf("TXCQ: %lu\n", tx_cq_cntr);
// 	ret = ft_get_tx_comp(tx_seq);
// 	if (ret)
// 		return ret;
// 	printf("TXCQ: %lu\n", tx_cq_cntr);

// 	ft_free_res();
// 	return -ret;
// }

int pingpong(void)
{
	int ret, i;

	ret = ft_sync();
	if (ret)
		return ret;

	if (opts.dst_addr)
	{
		for (i = 0; i < opts.iterations + opts.warmup_iterations; i++)
		{
			if (i == opts.warmup_iterations)
				ft_start();

			if (opts.transfer_size < fi->tx_attr->inject_size)
				ret = ft_inject(ep, remote_fi_addr, opts.transfer_size);
			else
				ret = ft_tx(ep, remote_fi_addr, opts.transfer_size, &tx_ctx);
			if (ret)
				return ret;

			ret = ft_rx(ep, opts.transfer_size);
			if (ret)
				return ret;
		}
	}
	else
	{
		for (i = 0; i < opts.iterations + opts.warmup_iterations; i++)
		{
			if (i == opts.warmup_iterations)
				ft_start();

			ret = ft_rx(ep, opts.transfer_size);
			if (ret)
				return ret;

			if (opts.transfer_size < fi->tx_attr->inject_size)
				ret = ft_inject(ep, remote_fi_addr, opts.transfer_size);
			else
				ret = ft_tx(ep, remote_fi_addr, opts.transfer_size, &tx_ctx);
			if (ret)
				return ret;
		}
	}
	ft_stop();

	if (opts.machr)
		show_perf_mr(opts.transfer_size, opts.iterations, &start, &end, 2,
					 opts.argc, opts.argv);
	else
		show_perf(NULL, opts.transfer_size, opts.iterations, &start, &end, 2);

	return 0;
}

int main1(int argc, char **argv)
{
	fi_info *hints = fi_allocinfo();

	if (!hints)
		return EXIT_FAILURE;

	hints->caps = FI_MSG;
	hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
	hints->domain_attr->threading = FI_THREAD_DOMAIN;
	hints->tx_attr->tclass = FI_TC_LOW_LATENCY;

	if (argc > 1)
	{
		libefa::Client client = libefa::Client("sockets", "rdm", "127.0.0.1", hints);
		client.init();
		client.initTxBuffer(1024);
		std::cout << tx_cq_cntr << std::endl;
		client.postTx();
		client.getTxCompletion();
		std::cout << tx_cq_cntr << std::endl;
	}
	else
	{
		libefa::Server server = libefa::Server("sockets", "rdm", hints);
		server.init();
		std::cout << rx_cq_cntr << std::endl;
		server.rx();
		std::cout << rx_cq_cntr << std::endl;
	}

	// opts.iterations = 10000;
	// opts.transfer_size = 1024;
	// pingpong();

	std::cout << "Test Done" << std::endl;
}