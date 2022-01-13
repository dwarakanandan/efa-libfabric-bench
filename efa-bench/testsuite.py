import paramiko
import subprocess
import time
import io
import sys

if len(sys.argv) > 1 and sys.argv[1] == 'local':
    SERVER_IP = '127.0.0.1'
    CLIENT_IP = '127.0.0.1'
    USERNAME = 'dwaraka'
    PASSWORD = 'dwarakacool007'
    EXECUTABLE_NAME = '/home/dwaraka/workspace/efa-libfabric-bench/build/benchmark'
    BASE_CONFIG = [
        '--debug', '--provider=sockets', '--hw_counters=/sys/class/net/lo/statistics/']
else:
    SERVER_IP = '172.31.27.197'
    CLIENT_IP = '172.31.25.149'
    USERNAME = 'ec2-user'
    PASSWORD = ''
    EXECUTABLE_NAME = '/home/ec2-user/workspace/efa-libfabric-bench/build/benchmark'
    BASE_CONFIG = [
        '--debug', '--provider=efa', '--hw_counters=/sys/class/infiniband/rdmap0s6/ports/1/hw_counters/']

RUNTIME = 2
PAYLOADS = [4, 64, 512, 1024, 4096, 8192, 16384, 65536]


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
    s_stdout, s_stderr = server.communicate()
    # print(server.returncode)

    killClient(c_stdin)


def runDGRAMPingPongTests():
    for payload in PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        config = ['--benchmark_type=ping_pong',
                  '--endpoint=dgram', payload_flag]
        stats_file = 'ping_pong_dgram_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runRDMPingPongTests():
    for payload in PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        config = ['--benchmark_type=ping_pong', '--endpoint=rdm', payload_flag]
        stats_file = 'ping_pong_rdm_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


def runRDMTaggedPingPongTests():
    for payload in PAYLOADS:
        payload_flag = '--payload=' + str(payload)
        config = ['--benchmark_type=ping_pong',
                  '--endpoint=rdm', '--tagged', payload_flag]
        stats_file = 'ping_pong_rdm_tagged_' + str(payload)
        runTestWithConfig(config, stats_file, RUNTIME)


if __name__ == "__main__":
    batch_config = ['--benchmark_type=batch', '--endpoint=rdm',
                    '--payload=1024', '--batch=100', '--tagged']
    rma_config = ['--benchmark_type=rma_sel_comp', '--endpoint=rdm',
                  '--payload=8192', '--batch=100', '--rma_op=write']
    runDGRAMPingPongTests()
    runRDMPingPongTests()
    runRDMTaggedPingPongTests()
