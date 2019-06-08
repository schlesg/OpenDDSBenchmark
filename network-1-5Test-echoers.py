import subprocess
import os

commandList = []
commandList.append("./echoer")
commandList.append("./echoer")
commandList.append("./echoer")
commandList.append("./echoer")
commandList.append("./echoer")


for command in commandList:
    subprocess.Popen(command.split(), cwd="build/")


input("Press Enter to shutdown...\n")
os.system("pkill -f echoer")
os.system("pkill -f initiator")
