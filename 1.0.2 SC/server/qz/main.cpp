#include <unistd.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>



using namespace std;

typedef struct
{
    int   buttons[15];
    float axes[6];
}JOY_MSG;

class JoyStick
{
public:
    JoyStick(void routine(int,siginfo_t* info,void *myact));
    ~JoyStick();
    void openJS(void routine(int,siginfo_t* info,void *myact));
    int  readJS(void routine(int,siginfo_t* info,void *myact));
private:
    js_event event;
    int      joy_fd;
    string   joy_dev_;

public:
    sigset_t block_mask;
    JOY_MSG  joy_msg;
    bool     work;
};

JoyStick::JoyStick(void routine(int,siginfo_t* info,void *myact))
{
    joy_dev_ = "/dev/input/js0";
    work     = true;
    openJS(routine);
}

JoyStick::~JoyStick()
{
    close(joy_fd);
}

void JoyStick::openJS(void routine(int,siginfo_t* info,void *myact))
{
    do{
        joy_fd = open(joy_dev_.c_str(), O_RDONLY|O_NONBLOCK);
        if( -1 == joy_fd )
            std::cerr<<"Couldn't open joystick "<<joy_dev_<<". Will retry every second."<<std::endl;
        sleep(1);
    }while( -1 == joy_fd );

    struct sigaction act;

    sigemptyset(&block_mask);
    sigaddset(&block_mask,SIGIO);
    act.sa_sigaction = routine;
    act.sa_mask      = block_mask;
    act.sa_flags     = SA_SIGINFO;
    if(sigaction(SIGIO,&act,NULL)<0)
        std::cerr<<"install sigal error"<<std::endl;

    fcntl(joy_fd,F_SETOWN,getpid());
    int oflags = fcntl(joy_fd,F_GETFL);
    fcntl(joy_fd,F_SETFL,oflags|FASYNC);
}

int JoyStick::readJS(void routine(int,siginfo_t* info,void *myact))
{
    work = false;

    float tmp;
    for(int i=0;i<4;i++)
    {
        if (read(joy_fd, &event, sizeof(js_event)) == -1 && errno != EAGAIN)
        {
            std::cerr<<strerror(errno)<<std::endl; // Joystick is probably closed. Definitely occurs.
            close(joy_fd);
            openJS(routine);
            work = true;
            return -1;
        }
        switch(event.type)
        {
        case JS_EVENT_BUTTON:
            joy_msg.buttons[event.number] = (event.value ? 1 : 0);
            break;
        case JS_EVENT_AXIS:
            tmp = event.value /32767.0;
            if( 0.05>tmp && -0.05<tmp  )
                tmp=0;
            joy_msg.axes[event.number] = -tmp;
            break;
        }
    }
    /*for(int i=0;i<15;i++)
        cerr<<joy_msg.buttons[i]<<' ';
    cerr<<endl;
    for(int i=0;i<6;i++)
        cerr<<joy_msg.axes[i]<<' ';
    cerr<<endl;*/

    work = true;
    return 1;
}

void act(int sig);
void sendMsg(int , siginfo_t*, void *);

JoyStick    JOYSOCKET(sendMsg);
struct sockaddr_in addr_server;
struct sockaddr_in addr_client;
int                socket_fd;
bool               alive = true;


void act(int sig)
{
    if( SIGUSR1 == sig)
        cerr<<"Disconnected"<<endl;
    //else if(SIGUSR2 == sig)
    //    cerr<<"Connected"<<endl;
    else if(SIGINT == sig){
        close(socket_fd);
        alive = false;
        printf("exit\n");
        exit(0);
    }
}

void sendMsg(int signalNo,siginfo_t* info,void *myact)
{
    if(signalNo == SIGIO)
    {
        sigprocmask(SIG_BLOCK, &JOYSOCKET.block_mask,NULL );
        if(JOYSOCKET.work )
        {
            int  i=JOYSOCKET.readJS(sendMsg);
            if( -1 != i )
                sendto(socket_fd,&JOYSOCKET.joy_msg,sizeof(JOYSOCKET.joy_msg),0,
                       (struct sockaddr*)&addr_client,sizeof(addr_client));
        }
        sigprocmask(SIG_UNBLOCK, &JOYSOCKET.block_mask,NULL );
    }
}

static void usage(FILE *fp,char ** argv){
    fprintf(fp,"Usage:%s[options]\n\n"
            "Options:\n"
            "-i | --ip          Ip of client\n"
            "-n | --name        PC name of client\n"
            "-h | --help        Print this message\n"
            "", argv[0]);
}

static const char short_options[] = "i:n:h";

static const struct option long_options[] = {
                        {"ip",  required_argument,NULL,'i'},
                        {"name",required_argument,NULL,'n'},
                        {"help",no_argument,NULL,'h'},
                        {0,0,0,0} };

int main(int argc, char *argv[])
{
    char *ip,*name;
    struct hostent *host = NULL;
    int index,c, selectIpOrName = 0;
    
cerr<<getpid()<<endl;

    while(1){
        c = getopt_long(argc, argv, short_options, long_options, &index);

        if(-1 == c)
            break;

        switch(c){
        case 0:
            break;
        case 'i':
            ip = optarg;
            selectIpOrName = 1;
            break;
        case 'n':
            name = optarg;
            selectIpOrName = 2;
            break;
        case 'h':
            usage(stdout,argv);
            exit(EXIT_FAILURE);
        default:
            usage(stderr,argv);
            exit(EXIT_FAILURE);
        }
    }
    socket_fd =  socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_server.sin_port = htons(8888);

    memset(&addr_client, 0, sizeof(addr_client));
    addr_client.sin_family = AF_INET;
switch(selectIpOrName){
case 0:
	addr_client.sin_addr.s_addr =  inet_addr("192.168.8.102");
        ip = inet_ntoa(addr_client.sin_addr);
        printf("Default dest ip %s\n",ip);
	break;
case 1:
	addr_client.sin_addr.s_addr = inet_addr(ip);//htonl(192.168.8.103);
        ip = inet_ntoa(addr_client.sin_addr);
        printf("Dest ip %s\n",ip);
	break;
case 2:
	host = gethostbyname(name);
        addr_client.sin_addr=*((struct in_addr *)host->h_addr);
        ip = inet_ntoa(addr_client.sin_addr);
        printf("Dest ip %s\n",ip);
	break;
}
    addr_client.sin_port = htons(8888);

    bind(socket_fd,(struct sockaddr*)&addr_server, sizeof(addr_server));


    
    signal(SIGUSR1,act);
    signal(SIGUSR2,act);
    signal(SIGINT, act);

    while(alive){
        sleep(1);
    }

    return 0;
}


