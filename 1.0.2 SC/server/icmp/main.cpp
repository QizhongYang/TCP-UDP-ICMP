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
static struct sockaddr_in dest;
static int alive;
static unsigned char recv_buff[2048];
static unsigned char send_buff[72];
u_int16_t seq=0;
static int timeout=0;
static int qzpid;


static unsigned short icmp_cksum(unsigned char *data, int len)
{
    int sum = 0; //计算结果
    int odd = len & 0x01; //是否为奇数
    /*将数据按照2字节为单位累加起来*/
    while(len & 0xfffe){
        sum += *(unsigned short*)data;
        data += 2;
        len -= 2;
    }
    /*判断是否为奇数个数据,若ICMP报头为奇数个字节,会剩下最后一个字节*/
    if(odd){
        unsigned short tmp = ((*data)<<8)&0xff00;
        sum += tmp;
    }
    sum = (sum >> 16) + (sum & 0xffff); //高地位相加
    sum += (sum >> 16); //将溢出位加入

    return ~sum; //返回取反值
}

static void icmp_pack(struct icmp *icmph,  int length)
{
    unsigned char i = 0;
    //设置报头
    icmph->icmp_type = ICMP_ECHO; //ICMP回显请求
    icmph->icmp_code = 0; //code的值为0
    icmph->icmp_cksum = 0; //先将cksum的值填为0，便于以后的cksum计算
    icmph->icmp_seq = seq++; //本报的序列号
    icmph->icmp_id = PID; //填写PID
    for(i=0; i< length; i++)
        icmph->icmp_data[i] = i; //计算校验和
    icmph->icmp_cksum = icmp_cksum((unsigned char*)icmph, length);
}

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
    //ICMP类型为ICMP_ECHOREPLY并且为本进程的PID
    if((icmp->icmp_type == ICMP_ECHOREPLY)
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

static void *icmp_recv(void *argv)
{
    pthread_detach(pthread_self());
    while(alive)
    {
        struct timeval tv;
        tv.tv_usec = 500000;
        tv.tv_sec = 0;
        fd_set readfd;
        int ret = 0;
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
                kill(qzpid,SIGUSR2);
            }
            break;
        }
    }
    if(argv == NULL)
        argv = NULL;
    return NULL;
}

int main(int argc, char *argv[])
{
    struct hostent  *host = NULL;
    struct protoent *protocol = NULL;


FILE *pf=popen("pidof qz","r");
char *res;
res = new char[5];
fread(res,5,1,pf);
pclose(pf);
std::stringstream ss;
ss<<res;
ss>>qzpid;
printf("%d\n",qzpid);
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


    //获取目的地址的IP地址
    dest.sin_family = AF_INET;
    //输入的目的地址为字符串IP地址
    in_addr_t inaddr;

    if(argc < 2)
    {
        inaddr = inet_addr("192.168.168.127");
    }else
        inaddr = inet_addr(argv[1]);
    if(inaddr == INADDR_NONE){ //输入的是DNS地址
        host = gethostbyname(argv[1]);
        if(host == NULL){
            perror("gethostbyname");
            return -1;
        }
        //将地址复制到dest
        memcpy((char *)&dest.sin_addr, host->h_addr, host->h_length);
    } //IP地址字符串
    else {
        memcpy((char *)&dest.sin_addr, &inaddr,sizeof(inaddr));
    }
    //打印提示
    inaddr = dest.sin_addr.s_addr;
    printf("PING %u.%u.%u.%u \n",
           (inaddr&0x000000ff)>>0,(inaddr&0x0000ff00)>>8,(inaddr&0x00ff0000)>>16,(inaddr&0xff000000)>>24);
    //截取信号SIGINT,将icmp_sigint挂接上
    signal(SIGINT,icmp_sigint);



    alive = true; //初始化可运行
    pthread_t  recv_id; //建立线程，用于接收

    int err = pthread_create(&recv_id, NULL, icmp_recv, NULL); //接收
    if(err < 0){
        exit(-1);
    }
    while(alive)
    {
        int size = 0;
        icmp_pack((struct icmp *)send_buff, 12);
        //打包数据
        size = sendto(rawsock, send_buff, 12, 0,
                      (struct sockaddr *)&dest, sizeof(dest));
        if(size < 0)
        {
            perror("sendto error");
            timeout>OUTCOUNT?:timeout++;
        }
        if(timeout >OUTCOUNT)
        {
            std::cerr<<seq<<" error timeout.\n";
            kill(qzpid,SIGUSR1);
        }
        //每隔0.5s发送一个ICMP回显请求包
        usleep(500000);
    }
    //等待线程结束
    pthread_join(recv_id, NULL);

    close(rawsock);
    printf("\nexit\n");
    exit(0);
}
