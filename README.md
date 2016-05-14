# TCP-UDP-ICMP
mkdir -p ~/Desktop/ws/src
cd ~/Desktop/ws/src/
catkin_init_workspace
cd ..
catkin_make
sudo gedit ~/.bashrc
source ~/Desktop/ws/devel/setup.bash
rospack profile 


cd ~/Desktop/ws/src 
catkin_create_pkg mypackage


g++ -o icmp -main.cpp -l pthread
