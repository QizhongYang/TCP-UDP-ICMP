#include <ros/ros.h>
#include <sensor_msgs/Joy.h>
#include <geometry_msgs/Twist.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <signal.h>
using namespace std;


ros::Publisher      pub ;
ros::Subscriber     sub ;
int GEAR=0;    
geometry_msgs::Twist JOYBUF;


void act(int sig)
{
    if(SIGINT == sig)
    {
	exit(0);
    }
}

void JoyMessageReceived(const sensor_msgs::Joy &msg)
{
    JOYBUF.linear.x=double(msg.axes[1]/5.0)*double(GEAR);
    JOYBUF.angular.z=double(msg.axes[3]);
    if(msg.buttons[4]>0.5)
    {
         GEAR>=5?GEAR=5:GEAR++;
	 std::cerr<<GEAR<<std::endl;
    }
    if(msg.buttons[5]>0.5)
    {
         if(GEAR>0)
             GEAR--;
	std::cerr<<GEAR<<std::endl;
    }
    if(msg.axes[5]<0.5||msg.axes[2]<0.5)
    {
         JOYBUF.linear.x=0;
         JOYBUF.angular.z=0;
         std::cerr<<"brake\n";
    }
    pub.publish(JOYBUF);

}

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "tbctr");
    ros::NodeHandle nh;
    sub  = nh.subscribe("joy",1000,&JoyMessageReceived);
    pub  = nh.advertise<geometry_msgs::Twist>("/cmd_vel_mux/input/teleop",1000);
        
    signal(SIGINT,act);
	
    std::cerr<<"begin\n";
    ros::Rate rate(10);
    while(nh.ok())
    {
	ros::spinOnce();
	
	pub.publish(JOYBUF);
	rate.sleep();
    }
    return 0;
}




