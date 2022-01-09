import paramiko
import subprocess
import time
import io

server_ip = '127.0.0.1'
client_ip = '127.0.0.1'
port = 22
username = 'dwaraka'
password = 'dwarakacool007'

executable_name = '/home/dwaraka/workspace/efa-libfabric-bench/build/benchmark'
base_config = '--debug --benchmark_type=batch --provider=sockets --endpoint=rdm --iterations=10000 --payload=64 --batch=100 --rma_op=write --tagged --hw_counters=/sys/class/net/lo/statistics/'


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


def startServer(cmd):
    return subprocess.Popen(
        cmd,
        shell=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )


def buildClientConfig(config, runtime):
    return executable_name + ' ' + \
        config + ' ' + \
        '--mode=client' + ' ' + \
        '--dst_addr=' + server_ip + ' ' + \
        '--runtime=' + str(runtime)


def buildServerConfig(config, runtime):
    return executable_name + ' ' + \
        config + ' ' + \
        '--mode=server' + ' ' + \
        '--runtime=' + str(runtime)


def runTestWithConfig(config):
    # Start server process
    server = startServer(buildServerConfig(config, 5))

    # Connect to client node and start client process
    client = getSSHClient()
    c_stdin, c_stdout = execOnClient(client, buildClientConfig(config, 7))

    # Wait till server completes
    s_stdout, s_stderr = server.communicate()
    # print(server.returncode)

    killClient(c_stdin)


runTestWithConfig(base_config)
