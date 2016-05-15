#! /bin/bash

mkdir -p ./ws/src
cd ./ws/src
catkin_init_workspace
cd ..
catkin_make 
source ./devel/setup.bash

cd ./src
catkin_create_pkg udp_js
catkin_create_pkg turtlebot
cd ..
cd ..
cd ./client/
cp -r ./turtlebot ../ws/src/
cp -r ./udp_js ../ws/src/

cd ..
cd ./ws/
catkin_make
source ./devel/setup.bash

roscore&


cd ..
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
sudo ./icmp

