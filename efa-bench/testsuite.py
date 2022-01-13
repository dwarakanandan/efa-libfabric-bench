import paramiko
import subprocess
import time
import io
import sys

if len(sys.argv) > 1 and sys.argv[1] == 'local':
    server_ip = '127.0.0.1'
    client_ip = '127.0.0.1'
    port = 22
    username = 'dwaraka'
    password = 'dwarakacool007'
    executable_name = '/home/dwaraka/workspace/efa-libfabric-bench/build/benchmark'
    base_config = [
        '--debug', '--provider=sockets', '--hw_counters=/sys/class/net/lo/statistics/']
else:
    server_ip = '172.31.27.197'
    client_ip = '172.31.25.149'
    port = 22
    username = 'ec2-user'
    password = ''
    executable_name = '/home/ec2-user/workspace/efa-libfabric-bench/build/benchmark'
    base_config = [
        '--debug', '--provider=efa', '--hw_counters=/sys/class/infiniband/rdmap0s6/ports/1/hw_counters/']


def getSSHClient():
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(client_ip, port, username, password)
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
    args.append(executable_name)
    args.extend(base_config)
    args.extend(config)
    args.append('--mode=client')
    args.append('--dst_addr=' + server_ip)
    args.append('--runtime=' + str(runtime))

    flatargs = ''
    for i in args:
        flatargs += ' ' + i
    return flatargs


def buildServerCmd(config, stat_file, runtime):
    args = []
    args.append(executable_name)
    args.extend(base_config)
    args.extend(config)
    args.append('--mode=server')
    args.append('--stat_file=' + stat_file)
    args.append('--runtime=' + str(runtime))
    print("Running config:", args)
    return args


def runTestWithConfig(config, stat_file):
    # Start server process
    server = startServer(buildServerCmd(config, stat_file, 5))

    # SSH to client node and start client process
    client = getSSHClient()
    c_stdin, c_stdout = execOnClient(client, buildClientCmd(config, 7))

    # Wait till server completes
    s_stdout, s_stderr = server.communicate()
    # print(server.returncode)

    killClient(c_stdin)


if __name__ == "__main__":
    batch_config = ['--benchmark_type=batch', '--endpoint=rdm',
                    '--payload=1024', '--batch=100', '--tagged']
    rma_config = ['--benchmark_type=rma_batch', '--endpoint=rdm',
                  '--payload=8192', '--batch=100', '--rma_op=write']
    runTestWithConfig(rma_config, 'rma1024')
