#! /bin/bash
ip='192.168.8.103'

cd ./server/camera 
cmake .
make


cd ..
cd ./icmp
g++ -o icmp main.cpp -l pthread

cd ..
cd ./qz
g++ -o qz main.cpp 


./qz -i $ip &
sleep 3

cd ..
cd ./icmp 
sudo ./icmp  $ip

 
