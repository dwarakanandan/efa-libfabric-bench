import paramiko
import subprocess
import time
import io

server_ip = '172.31.27.197'
client_ip = '172.31.25.149'
port = 22
username = 'ec2-user'
password = ''

executable_name = '/home/ec2-user/workspace/efa-libfabric-bench/build/benchmark'
base_config = ['--debug', '--benchmark_type=batch', '--provider=efa', '--endpoint=rdm',
               '--payload=1024', '--batch=100', '--rma_op=write', '--tagged',
               '--hw_counters=/sys/class/infiniband/rdmap0s6/ports/1/hw_counters/']


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
    stdin.write("\x03".encode())
    stdin.channel.close()


def startServer(args):
    return subprocess.Popen(
        args,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )


def buildClientCmd(baseconfig, runtime):
    args = []
    args.append(executable_name)
    args.extend(baseconfig)
    args.append('--mode=client')
    args.append('--dst_addr=' + server_ip)
    args.append('--runtime=' + str(runtime))

    flatargs = ''
    for i in args:
        flatargs += ' ' + i
    return flatargs


def buildServerCmd(baseconfig, runtime):
    args = []
    args.append(executable_name)
    args.extend(baseconfig)
    args.append('--mode=server')
    args.append('--runtime=' + str(runtime))
    return args


def runTestWithConfig(config):
    # Start server process
    server = startServer(buildServerCmd(config, 5))

    # SSH to client node and start client process
    client = getSSHClient()
    c_stdin, c_stdout = execOnClient(client, buildClientCmd(config, 7))

    # Wait till server completes
    s_stdout, s_stderr = server.communicate()
    # print(server.returncode)

    killClient(c_stdin)


runTestWithConfig(base_config)
