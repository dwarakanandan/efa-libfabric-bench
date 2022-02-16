#!/usr/bin/env python3
from statistics import mean

DIR_PREFIX = '.'
PAYLOADS = [4, 64, 512, 1024, 4096, 8192]
JUMBO_PAYLOADS = [4, 64, 512, 1024, 4096, 8192, 16384, 65536]
BATCH_SIZES = [1, 10, 50, 80, 100, 120]
RMA_BATCH_SIZES = [1, 10, 100, 200, 300, 500]

MT_DGRAM_PAYLOADS = [64, 8192]
MT_BATCH_SIZES = [2, 10, 50, 80, 100, 120]
MT_JUMBO_PAYLOADS = [64, 1024, 8192, 65536]
THREAD_COUNTS = [1, 2, 4, 8, 16, 32]


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
    if (len(list) == 0):
        return
    return round(mean(list), 2)


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


def parse_batch_tx_bw(benchmark, BATCH_LIST):
    print(benchmark)
    for batch in BATCH_LIST:
        bw_for_batch = []
        for payload in JUMBO_PAYLOADS:
            f_name = DIR_PREFIX + '/' + benchmark + '_' + \
                str(batch) + 'b_' + str(payload) + '.csv'
            ret = parse_file(f_name, 12)
            bw_for_batch.append(ret)
        print(batch, end=' ')
        for bw in bw_for_batch:
            print(bw, end=' ')
        print()
    print()


def parse_batch_rx_bw(benchmark, BATCH_LIST):
    print(benchmark)
    for batch in BATCH_LIST:
        bw_for_batch = []
        for payload in JUMBO_PAYLOADS:
            f_name = DIR_PREFIX + '/' + benchmark + '_' + \
                str(batch) + 'b_' + str(payload) + '.csv'
            ret = parse_file(f_name, 13)
            bw_for_batch.append(ret)
        print(batch, end=' ')
        for bw in bw_for_batch:
            print(bw, end=' ')
        print()
    print()


def parse_batch_app_bw(benchmark, BATCH_LIST):
    print(benchmark)
    for batch in BATCH_LIST:
        bw_for_batch = []
        for payload in JUMBO_PAYLOADS:
            f_name = DIR_PREFIX + '/' + benchmark + '_' + \
                str(batch) + 'b_' + str(payload) + '.csv'
            ret = parse_file(f_name, 14)
            bw_for_batch.append(ret)
        print(batch, end=' ')
        for bw in bw_for_batch:
            print(bw, end=' ')
        print()
    print()


def parse_mt_tx_bw_latency(benchmark, THREAD_LIST):
    print(benchmark)
    for thread_count in THREAD_LIST:
        bw_for_thread_count = []
        latency_for_thread_count = []
        for payload in MT_DGRAM_PAYLOADS:
            f_name = DIR_PREFIX + '/' + benchmark + '_' + \
                str(thread_count) + 't_' + str(payload) + '.csv'
            ret = parse_file(f_name, 12)
            bw_for_thread_count.append(ret)
            ret = parse_file(f_name, 15)
            latency_for_thread_count.append(ret)
        print(thread_count, end=' ')
        for i in range(0, len(MT_DGRAM_PAYLOADS)):
            print(bw_for_thread_count[i], end=' ')
            print(latency_for_thread_count[i], end=' ')
        print()
    print()


def parse_mt_tx_bw_latency_jumbo(benchmark, THREAD_LIST):
    print(benchmark)
    for thread_count in THREAD_LIST:
        bw_for_thread_count = []
        latency_for_thread_count = []
        for payload in MT_JUMBO_PAYLOADS:
            f_name = DIR_PREFIX + '/' + benchmark + '_' + \
                str(thread_count) + 't_' + str(payload) + '.csv'
            ret = parse_file(f_name, 12)
            bw_for_thread_count.append(ret)
            ret = parse_file(f_name, 15)
            latency_for_thread_count.append(ret)
        print(thread_count, end=' ')
        for i in range(0, len(MT_JUMBO_PAYLOADS)):
            print(bw_for_thread_count[i], end=' ')
            print(latency_for_thread_count[i], end=' ')
        print()
    print()


def parse_mt_batch_tx_bw_latency_jumbo(benchmark, THREAD_LIST, BATCH_LIST):
    print(benchmark)
    for thread_count in THREAD_LIST:
        for batch in BATCH_LIST:
            bw_for_thread_count_for_batch = []
            latency_for_thread_count_for_batch = []
            for payload in MT_JUMBO_PAYLOADS:
                f_name = DIR_PREFIX + '/' + benchmark + '_' + str(batch) + 'b_' +\
                    str(thread_count) + 't_' + str(payload) + '.csv'
                ret = parse_file(f_name, 12)
                bw_for_thread_count_for_batch.append(ret)
                ret = parse_file(f_name, 15)
                latency_for_thread_count_for_batch.append(ret)
            print(str(thread_count) + 'x' + str(batch), end=' ')
            print(thread_count*batch, end=' ')
            for i in range(0, len(MT_JUMBO_PAYLOADS)):
                print(bw_for_thread_count_for_batch[i], end=' ')
                print(latency_for_thread_count_for_batch[i], end=' ')
            print()


def parse_file_matching_fields(f_name, experiment, endpoint, payload_size, batch_size, avg_field):
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
        if (experiment in split[1]) and \
            (endpoint in split[3]) and \
            (payload_size == int(split[8])) and \
                (batch_size == int(split[5])):
            list.append(float(split[avg_field]))
    if (len(list) == 0):
        return
    return round(mean(list), 2)


def parse_ping_pong():
    print("\nDGRAM:")
    for payload in PAYLOADS:
        avg = parse_file_matching_fields(
            "ping_pong.csv", "ping_pong", "dgram", payload, 1, 14)
        print(payload, avg)

    print("\nRDM:")
    for payload in JUMBO_PAYLOADS:
        avg = parse_file_matching_fields(
            "ping_pong.csv", "ping_pong", "rdm", payload, 1, 14)
        print(payload, avg)

    print("\nRDM tagged:")
    for payload in JUMBO_PAYLOADS:
        avg = parse_file_matching_fields(
            "ping_pong.csv", "ping_pong_tagged", "rdm", payload, 1, 14)
        print(payload, avg)


def parse_batch():
    print("\nDGRAM:")
    for payload in PAYLOADS:
        for batch in BATCH_SIZES:
            avg = parse_file_matching_fields(
                "batch.csv", "batch", "dgram", payload, batch, 14)
            print(payload, batch, avg)

    print("\nRDM:")
    for payload in JUMBO_PAYLOADS:
        for batch in BATCH_SIZES:
            avg = parse_file_matching_fields(
                "batch.csv", "batch", "rdm", payload, batch, 14)
            print(payload, batch, avg)

    print("\nRDM sel_comp:")
    for payload in JUMBO_PAYLOADS:
        for batch in BATCH_SIZES:
            avg = parse_file_matching_fields(
                "batch.csv", "batch_sel_comp", "rdm", payload, batch, 14)
            print(payload, batch, avg)


if __name__ == "__main__":
    # parse_tx_bw('ping_pong_dgram')
    # parse_tx_bw_jumbo('ping_pong_rdm')
    # parse_tx_bw_jumbo('ping_pong_rdm_tagged')

    # parse_rx_bw_jumbo('rma_read')
    # parse_tx_bw_jumbo('rma_write')

    # parse_rx_bw_jumbo('rma_sel_comp_read')
    # parse_tx_bw_jumbo('rma_sel_comp_write')

    # parse_batch_tx_bw('batch_dgram', BATCH_SIZES)
    # parse_batch_app_bw('batch_dgram', BATCH_SIZES)
    # parse_batch_tx_bw('batch_rdm', BATCH_SIZES)
    # parse_batch_app_bw('batch_rdm', BATCH_SIZES)

    # parse_batch_tx_bw('rma_batch_write', RMA_BATCH_SIZES)
    # parse_batch_rx_bw('rma_batch_read', RMA_BATCH_SIZES)

    # parse_mt_tx_bw_latency('ping_pong_dgram', THREAD_COUNTS)
    # parse_mt_tx_bw_latency_jumbo('ping_pong_rdm', THREAD_COUNTS)

    # parse_mt_tx_bw_latency_jumbo('rma_write', THREAD_COUNTS)

    # parse_mt_batch_tx_bw_latency_jumbo(
    #     'batch_rdm', THREAD_COUNTS, MT_BATCH_SIZES)
    # parse_mt_batch_tx_bw_latency_jumbo(
    #     'rma_batch_write', THREAD_COUNTS, MT_BATCH_SIZES)

    parse_ping_pong()
    parse_batch()
    pass
