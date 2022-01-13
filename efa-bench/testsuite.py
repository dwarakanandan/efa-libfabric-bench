import paramiko
import subprocess
import time
import io
import sys
from threading import Timer

'''
    Usage: python3 testsuite.py <server-ip> <client-ip> [local]
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
    print('Running config:', args, '\n')
    return args


def runTestWithConfig(config, stat_file, runtime):
    # Start server process
    server = startServer(buildServerCmd(config, stat_file, runtime))

    # SSH to client node and start client process
    client = getSSHClient()
    c_stdin, c_stdout = execOnClient(client, buildClientCmd(config, runtime+2))

    # Wait till server completes
    timer = Timer(RUNTIME + 5, server.kill)
    try:
        timer.start()
        s_stdout, s_stderr = server.communicate()
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
        config = ['--benchmark_type=batch',
                  '--endpoint=rdm', '--tagged', batch_flag, payload_flag]
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
        config = ['--benchmark_type=rma_batch',
                  '--endpoint=rdm', rma_flag, batch_flag, payload_flag]
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
    runPingPongDGRAM()
    runPingPongRDM()
    runPingPongRDMTagged()
    runBatchDGRAM(100)
    runBatchRDM(100)
    runBatchRDMTagged(100)
    runRMA('write')
    runRMA('read')
    runRMABatch('write', 100)
    runRMABatch('read', 100)
    runRMASelectiveCompletion('write')
    runRMASelectiveCompletion('read')
