#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>

#define OUTCOUNT 2
#define PID      0xFA64

static int rawsock = 0;
static int alive;
static unsigned char recv_buff[2048];
static int timeout=0;
static int mainpid;


static int icmp_unpack( unsigned char *buf, int len)
{
    int iphdrlen;
    struct ip *ip = NULL;
    struct icmp *icmp = NULL;

    ip = (struct ip *)buf; //IP报头
    iphdrlen = ip->ip_hl * 4; //IP头部长度
    icmp = (struct icmp *)(buf+iphdrlen); //ICMP段的地址
    len -= iphdrlen;
    //判断长度是否为ICMP包
    if(len < 8){
        printf("ICMP packets\'s length is less than 8\n");
        return -1;
    }
    //ICMP类型为ICMP_ECHOREPLY
    if((icmp->icmp_type == ICMP_ECHO)
            && (icmp->icmp_id == PID))
         return 0;
    else if((icmp->icmp_type == ICMP_DEST_UNREACH)
            && (icmp->icmp_id == PID))
         return -1;
    return -1;
}

void icmp_sigint(int signo)
{
    if( SIGINT == signo )
        alive = false;
}



int main(int argc, char *argv[])
{
    struct protoent *protocol = NULL;

FILE *pf=popen("pidof udp_js","r");
char *res;
res = new char[5];
fread(res,5,1,pf);
pclose(pf);
std::stringstream ss;
ss<<res;
ss>>mainpid;
printf("%d\n",mainpid);
delete[] res;


    protocol = getprotobyname("icmp");
    if(protocol == NULL)
    {
        perror("getprotobyname()");
        return -1;
    }

    //socket初始化
    rawsock = socket(AF_INET, SOCK_RAW, protocol->p_proto);
    if( socket(AF_INET, SOCK_RAW, protocol->p_proto) < 0 )
    {
        perror("socket");
        return -1;
    }

    //截取信号SIGINT,将icmp_sigint挂接上
    signal(SIGINT,icmp_sigint);

    alive = true; //初始化可运行
    struct timeval tv;
    int ret = 0;

    while(alive)
    {

        tv.tv_usec = 0;
        tv.tv_sec = 1;
        fd_set readfd;
        FD_ZERO(&readfd);
        FD_SET(rawsock,&readfd);

        ret = select(rawsock+1,&readfd,NULL,NULL,&tv);
        switch(ret)
        {
        case -1:
            //错误发生
            break;
        case 0:
            //超时
            timeout>OUTCOUNT?:timeout++;
            break;
        default :
            //接收数据
            int size = recv(rawsock,recv_buff,sizeof(recv_buff),0);
            if(errno == EINTR)
            {
                perror("recvfrom error");
                break;
            }
            //解包
            if( icmp_unpack(recv_buff,size) == -1 )
            {
                timeout>OUTCOUNT?:timeout++;
                break;
            }else
            {
                timeout=0;
                kill(mainpid,SIGUSR2);
            }
            break;
        }
        if(timeout>OUTCOUNT)
        {
            std::cerr<<"time out.\n";
            kill(mainpid,SIGUSR1);
        }
    }

    close(rawsock);
    printf("\nexit\n");
    exit(0);
}
