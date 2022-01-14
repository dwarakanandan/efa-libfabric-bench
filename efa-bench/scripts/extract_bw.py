#!/usr/bin/env python3

PAYLOADS = [4, 64, 512, 1024, 4096, 8192]
JUMBO_PAYLOADS = [4, 64, 512, 1024, 4096, 8192, 16384, 65536]
DIR_PREFIX = '.'

def average(l):
    return round(sum(l) / len(l), 2)


def parse_file(f_name, field):
    try:
        file = open(f_name, "r")
    except FileNotFoundError:
        return
    list = []
    read_header = False
    for line in file:
        if not read_header:
            read_header = True
            continue
        split = line.rstrip().split(',')
        list.append(float(split[field]))
    return average(list)


def parse_tx_bw(benchmark):
    print(benchmark)
    for payload in PAYLOADS:
        f_name = DIR_PREFIX + '/' + benchmark + '_' + str(payload) + '.csv'
        ret = parse_file(f_name, 12)
        print(payload, ret)
    print()


def parse_tx_bw_jumbo(benchmark):
    print(benchmark)
    for payload in JUMBO_PAYLOADS:
        f_name = DIR_PREFIX + '/' + benchmark + '_' + str(payload) + '.csv'
        ret = parse_file(f_name, 12)
        print(payload, ret)
    print()


def parse_rx_bw_jumbo(benchmark):
    print(benchmark)
    for payload in JUMBO_PAYLOADS:
        f_name = DIR_PREFIX + '/' + benchmark + '_' + str(payload) + '.csv'
        ret = parse_file(f_name, 13)
        print(payload, ret)
    print()


if __name__ == "__main__":
    # parse_tx_bw('ping_pong_dgram')
    # parse_tx_bw_jumbo('ping_pong_rdm')
    # parse_tx_bw_jumbo('ping_pong_rdm_tagged')

    # parse_tx_bw('batch_rdm_100b')
    parse_tx_bw('batch_rdm_tagged_100b')

    # parse_rx_bw_jumbo('rma_read')
    # parse_tx_bw_jumbo('rma_write')

    # parse_rx_bw_jumbo('rma_batch_read_100b')
    # parse_tx_bw_jumbo('rma_batch_write_100b')

    # parse_rx_bw_jumbo('rma_sel_comp_read')
    # parse_tx_bw_jumbo('rma_sel_comp_write')
