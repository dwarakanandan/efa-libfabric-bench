#!/usr/bin/env python3

import paramiko
import subprocess
import sys
from threading import Timer

'''
    Usage: test_runner.py <server-ip> <client-ip> [local]
    TEST_CASE_COUTN = 92
    RUNTIME_PER_CASE = 30sec
    TOTAL_RUNTIME = ~45min
'''

if len(sys.argv) > 3 and sys.argv[3] == 'local':
    SERVER_IP = '127.0.0.1'
    CLIENT_IP = '127.0.0.1'
    USERNAME = 'dwaraka'
    PASSWORD = 'dwarakacool007'
    EXECUTABLE_NAME = '/home/dwaraka/workspace/efa-libfabric-bench/build/benchmark'
    BASE_CONFIG = [
        '--debug', '--provider=sockets', '--hw_counters=/sys/class/net/lo/statistics/']
else:
    SERVER_IP = sys.argv[1]
    CLIENT_IP = sys.argv[2]
    USERNAME = 'ec2-user'
    PASSWORD = ''
    EXECUTABLE_NAME = '/home/ec2-user/workspace/efa-libfabric-bench/build/benchmark'
    BASE_CONFIG = [
        '--debug', '--provider=efa', '--hw_counters=/sys/class/infiniband/rdmap0s6/ports/1/hw_counters/']

RUNTIME = 1
DGRAM_PAYLOADS = [4, 64, 512, 1024, 4096, 8192]
RDM_PAYLOADS = [4, 64, 512, 1024, 4096, 8192, 16384, 65536]

PAYLOAD_ITERATION_MAP = {
    4: 100000,
    64: 100000,
    512: 100000,
    1024: 100000,
    4096: 100000,
    8192: 100000,
    16384: 50000,
    65536: 50000
}


def getSSHClient():
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(CLIENT_IP, 22, USERNAME, PASSWORD)
    return ssh


def execOnClient(client, cmd):
    stdin, stdout, stderr = client.exec_command(cmd, get_pty=True)
    # exit_status = stdout.channel.recv_exit_status()
    # print('Exec cmd status: ' + str(exit_status))
    return stdin, stdout


def killClient(stdin):
    try:
        stdin.write("\x03".encode())
        stdin.channel.close()
    except OSError:
        # print('Client socket closed')
        return


def startServer(args):
    return subprocess.Popen(
        args,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )


def buildClientCmd(config, runtime):
    args = []
    args.append(EXECUTABLE_NAME)
    args.extend(BASE_CONFIG)
    args.extend(config)
    args.append('--mode=client')
    args.append('--dst_addr=' + SERVER_IP)
    args.append('--runtime=' + str(runtime))

    flatargs = ''
    for i in args:
        flatargs += ' ' + i
    return flatargs


def buildServerCmd(config, stat_file, runtime):
    args = []
    args.append(EXECUTABLE_NAME)
    args.extend(BASE_CONFIG)
    args.extend(config)
    args.append('--mode=server')
    args.append('--stat_file=' + stat_file)
    args.append('--runtime=' + str(runtime))
    return args


def runTestWithConfig(config, stat_file, runtime, kill_timeout=True, print_cmd=True, print_stdout=False):
    # Start server process
    server_cmd = buildServerCmd(config, stat_file, runtime)
    if print_cmd:
        print('Running config:', server_cmd, '\n')
    server = startServer(server_cmd)

    # SSH to client node and start client process
    client = getSSHClient()
    c_stdin, c_stdout = execOnClient(client, buildClientCmd(config, runtime+2))

    # Wait till server completes
    timeout = RUNTIME + 5 if kill_timeout else 3600
    timer = Timer(timeout, server.kill)
    try:
        timer.start()
        s_stdout, s_stderr = server.communicate()
        if print_stdout:
            print(s_stdout.decode("utf-8"))
    finally:
        if not timer.is_alive():
            print('Server process killed due to timeout...')
        timer.cancel()
    if server.returncode != 0:
        print('Server exited with code:', server.returncode)
    killClient(c_stdin)


def runPingPongDGRAM():
    for payload in DGRAM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        config = ['--benchmark_type=ping_pong',
                  '--endpoint=dgram', payload_flag]
        stats_file = 'ping_pong_dgram_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runPingPongRDM():
    for payload in RDM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        config = ['--benchmark_type=ping_pong', '--endpoint=rdm', payload_flag]
        stats_file = 'ping_pong_rdm_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runPingPongRDMTagged():
    for payload in RDM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        config = ['--benchmark_type=ping_pong',
                  '--endpoint=rdm', '--tagged', payload_flag]
        stats_file = 'ping_pong_rdm_tagged_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runLatencyDGRAM():
    print('latency_dgram')
    for payload in DGRAM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        iterations_flag = '--iterations=' + str(PAYLOAD_ITERATION_MAP[payload])
        config = ['--benchmark_type=latency',
                  '--endpoint=dgram', payload_flag, iterations_flag]
        stats_file = 'latency_dgram_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME, False, False, True)
    print('\n')


def runLatencyRDM():
    print('latency_rdm')
    for payload in RDM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        iterations_flag = '--iterations=' + str(PAYLOAD_ITERATION_MAP[payload])
        config = ['--benchmark_type=latency',
                  '--endpoint=rdm', payload_flag, iterations_flag]
        stats_file = 'latency_rdm_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME, False, False, True)
    print('\n')


def runLatencyRDMTagged():
    print('latency_rdm_tagged')
    for payload in RDM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        iterations_flag = '--iterations=' + str(PAYLOAD_ITERATION_MAP[payload])
        config = ['--benchmark_type=latency',
                  '--endpoint=rdm', '--tagged', payload_flag, iterations_flag]
        stats_file = 'latency_rdm_tagged_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME, False, False, True)
    print('\n')


def runBatchDGRAM(batch):
    for payload in DGRAM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        batch_flag = '--batch=' + str(batch)
        config = ['--benchmark_type=batch',
                  '--endpoint=dgram', batch_flag, payload_flag]
        stats_file = 'batch_dgram_' + str(batch) + 'b_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runBatchRDM(batch):
    for payload in RDM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        batch_flag = '--batch=' + str(batch)
        config = ['--benchmark_type=batch',
                  '--endpoint=rdm', batch_flag, payload_flag]
        stats_file = 'batch_rdm_' + str(batch) + 'b_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runBatchRDMTagged(batch):
    for payload in RDM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        batch_flag = '--batch=' + str(batch)
        cq_try_flag = '--cq_try=' + str(0.8 if payload <= 8192 else 0.9)
        config = ['--benchmark_type=batch',
                  '--endpoint=rdm', '--tagged', batch_flag, cq_try_flag, payload_flag]
        stats_file = 'batch_rdm_tagged_' + str(batch) + 'b_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runRMA(rma_op):
    for payload in RDM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        rma_flag = '--rma_op=' + rma_op
        config = ['--benchmark_type=rma',
                  '--endpoint=rdm', rma_flag, payload_flag]
        stats_file = 'rma_' + rma_op + '_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runRMABatch(rma_op, batch):
    for payload in RDM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        batch_flag = '--batch=' + str(batch)
        rma_flag = '--rma_op=' + rma_op
        cq_try_flag = '--cq_try=' + str(0.8 if payload < 8192 else 0.9)
        config = ['--benchmark_type=rma_batch',
                  '--endpoint=rdm', rma_flag, batch_flag, payload_flag, cq_try_flag]
        stats_file = 'rma_batch_' + rma_op + \
            '_' + str(batch) + 'b_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runRMASelectiveCompletion(rma_op):
    for payload in RDM_PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        rma_flag = '--rma_op=' + rma_op
        config = ['--benchmark_type=rma_sel_comp',
                  '--endpoint=rdm', rma_flag, payload_flag]
        stats_file = 'rma_sel_comp_' + rma_op + '_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


if __name__ == "__main__":
    # runPingPongDGRAM()
    # runPingPongRDM()
    # runPingPongRDMTagged()
    # runRMA('write')
    # runRMA('read')
    # runRMASelectiveCompletion('write')
    # runRMASelectiveCompletion('read')

    # runLatencyDGRAM()
    # runLatencyRDM()
    # runLatencyRDMTagged()

    # runBatchRDMTagged(2)
    # runBatchRDMTagged(10)
    # runBatchRDMTagged(50)
    # runBatchRDMTagged(80)
    # runBatchRDMTagged(100)
    # runBatchRDMTagged(120)

    # runRMABatch('write', 2)
    # runRMABatch('write', 10)
    # runRMABatch('write', 100)
    # runRMABatch('write', 200)
    # runRMABatch('write', 300)
    # runRMABatch('write', 500)
    pass