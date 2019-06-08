import subprocess
import os

# print(os.getcwd())
# os.chdir("build/")
commandList = []

msgLen = '10000'
commandList.append("./initiator --msgLength " + msgLen)
commandList.append("./initiator --pubName Dummy --msgLength " + msgLen)
commandList.append("./initiator --pubName Dummy --msgLength " + msgLen)
commandList.append("./initiator --pubName Dummy --msgLength " + msgLen)
commandList.append("./initiator --pubName Dummy --msgLength " + msgLen)

for command in commandList:
    subprocess.Popen(command.split(), cwd="build/")


input("Press Enter to shutdown...\n")
os.system("pkill -f echoer")
os.system("pkill -f initiator")
