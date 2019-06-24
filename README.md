# DDS Benchmark

OpenDDS ping-pong latency tester

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

#### OpenDDS 
git clone https://github.com/objectcomputing/OpenDDS.git  
./configure --ace=download --ace-github-latest --no-tests (optional)
. ./setenv.sh  
make  
bin/all_tests.pl (optional)  

#### Boost  
   wget -O boost_1_69_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.69.0/boost_1_69_0.tar.gz/download  
   tar xzvf boost_1_69_0.tar.gz  
   cd boost_1_69_0  
   ./bootstrap.sh  
   ./b2  
   
#### Python3  
apt install python3

#### Cmake  
sudo apt purge cmake (remove current cmake version)  
download latest cmake (at least 3.8) from https://cmake.org/download/.  
./configure  
make  
make install  


### Installing
. $DDS_ROOT/setenv.sh  
git clone https://github.com/schlesg/OpenDDSBenchmark.git  
mkdir build  
cd build  
cmake -DBOOST_ROOT={BoostRootDir/} ..  
make  

## Running the tests
. $DDS_ROOT/setenv.sh  
* Can run the Initiator and Echoer apps directly:
  - ./initiator --msgLength 1000  
* More complex test are defined within the py files (e.g. IPC-1-5Test.py).

In case MC is not supported - you will need to add SpdpSendAddrs={RemoteHostIP}:{RemoteHostPort} to the rtps_disc.ini
In case running SHMEM configuration, you will need to tun the DCPSInfoRepo first - $DDS_ROOT/bin/DCPSInfoRepo

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

