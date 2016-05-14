#! /bin/bash
source ./ws/devel/setup.bash

cd ./client/icmpClient
g++ -o icmp main.cpp
cd ..
cd ./camera/
cmake .
make

cd ..
cd ./icmpClient

rosrun udp_js udp_js&
sleep 3
./icmp

