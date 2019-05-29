import subprocess
import os

# print(os.getcwd())
# os.chdir("build/")
commandList = []
commandList.append("./echoer --Trans shmem.ini")
commandList.append("./echoer --Trans shmem.ini")
commandList.append("./echoer --Trans shmem.ini")
commandList.append("./echoer --Trans shmem.ini")
commandList.append("./echoer --Trans shmem.ini")
commandList.append("./initiator --Trans shmem.ini --subCount 5 --msgLength 10000")


# commandList.append("./initiator --SubTopic C5 --PubTopic MS1 --msgLength 5000 --roundtripCount 1000") #C5

for command in commandList:
    subprocess.Popen(command.split(), cwd="build/")


input("Press Enter to shutdown...\n")
os.system("pkill -f echoer")
os.system("pkill -f initiator")
