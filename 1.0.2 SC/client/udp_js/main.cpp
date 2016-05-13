#include <ros/ros.h>
#include <sensor_msgs/Joy.h>
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

typedef struct
{
    int   buttons[15];
    float axes[6];
}JOY_MSG;
sensor_msgs::Joy joy_msg;
ros::Publisher      pub ;
int s;
    

void act(int sig)
{
    if( SIGUSR1 == sig)
    {
        joy_msg.axes[5]=joy_msg.axes[2]=-1;
        joy_msg.header.stamp = ros::Time().now();
        pub.publish(joy_msg);
        cerr<<"stop\n";

    }else if( SIGUSR2 == sig)
    {
        //cerr<<"active\n";
    }else if(SIGINT == sig)
    {
	close(s);
	exit(0);
    }
}

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "js0");
    ros::NodeHandle nh;
    pub = nh.advertise<sensor_msgs::Joy>("joy",1);

    joy_msg.buttons.resize(16);
    joy_msg.axes.resize(6);


    std::cerr<<getpid()<<std::endl;

    signal(SIGUSR1,act);
    signal(SIGUSR2,act);
    signal(SIGINT,act);
	
    struct sockaddr_in addr_server,addr_client;

    s = socket(AF_INET,SOCK_DGRAM,0);

    memset(&addr_client, 0, sizeof(addr_client));
    addr_client.sin_family = AF_INET;
    addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_client.sin_port = htons(8888);



    bind(s,(struct sockaddr*)&addr_client,sizeof(addr_client));


    socklen_t len;
    JOY_MSG tmp;


    while(nh.ok())
    {
        len = sizeof(addr_server);
        recvfrom(s,&tmp, sizeof(tmp), 0, (struct sockaddr*)&addr_server, &len);
        for(int i=0;i<15;i++)
        {
            //std::cerr<<tmp.buttons[i]<<' ';
            joy_msg.buttons[i]=tmp.buttons[i];
        }
        //std::cerr<<std::endl;
        for(int i=0;i<6;i++)
        {
            //std::cerr<<tmp.axes[i]<<' ';
            joy_msg.axes[i]=tmp.axes[i];
        }
        //std::cerr<<std::endl;
        joy_msg.header.stamp = ros::Time().now();
        pub.publish(joy_msg);

    }
    close(s);
    return 0;
}
