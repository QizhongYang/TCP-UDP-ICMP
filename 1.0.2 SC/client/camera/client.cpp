#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_ntoa()函数的头文件
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <opencv2/opencv.hpp>
#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

using namespace cv;
#define portnumber 3333 //定义端口号：（0-1024为保留端口号，最好不要用）
VideoCapture cap(1);
VideoCapture cap1(0);
VideoCapture cap2(2);

ssize_t readn(int fd,char * vptr,size_t n)
{
    ssize_t nleft;
    ssize_t nread;
    char *ptr=vptr;
    nleft=n;
    while(nleft>0)
    {
        if(  (nread=read(fd,ptr,nleft))<0 )
        {
            if(errno ==EINTR)
                nread=0;
            else
                return -1;
        }
        else if(nread==0)
            break;
        nleft=nleft-nread;
        ptr+=nread;
    }
    return (n-nleft);
}

ssize_t writen(int fd,char * vptr,size_t n)
{
    ssize_t nleft;
    ssize_t nwrite;
    const char *ptr=vptr;
    nleft=n;
    while(nleft>0)
    {
        if(      (nwrite=write(fd,ptr,nleft))<0  )
        {
            if(errno ==EINTR)
                nwrite=0;
            else
                return -1;
        }
        nleft=nleft-nwrite;
        ptr+=nwrite;
    }
    return (n-nleft);
}

void str_echo(int connfd)
{
    ssize_t n;
    int maxfdp;
    fd_set fdset;
    timeval timeout={3,0};
    Mat frame;
    Mat frame1;
    Mat frame2;
    int i;
    filebuf *pbuf;
    ifstream filestr;
    long size;
    char *buffer;
    char SIZE[10];




    Mat roi;
    std::stringstream ss;

    cap>>frame;
    cap1>>frame1;
    cap2>>frame2;

    IplImage pCap= IplImage(frame);
    Size sz;
    sz.width =960;
    sz.height=480;
    IplImage *ptmp=cvCreateImage(sz,pCap.depth,pCap.nChannels);
    Mat bak(ptmp);


    sz.width =480;
    sz.height=240;
    IplImage *prr=cvCreateImage(sz,pCap.depth,pCap.nChannels);
    Mat prrbak(prr);

    vector<int> vct;
    vct.push_back(CV_IMWRITE_JPEG_QUALITY);
    vct.push_back(70);
    buffer=new char[120000];

    while(1)
    {
        i=0;
        cap>>frame;
        cap1>>frame1;
        cap2>>frame2;
        roi= bak(Rect(0,0,frame.cols,frame.rows));
        frame.copyTo(roi);
        roi= bak(Rect(frame.cols,0,frame1.cols,frame1.rows));
        frame1.copyTo(roi);
        roi= bak(Rect(frame.cols,frame1.rows,frame1.cols,frame1.rows));
        frame2.copyTo(roi);
        cvResize(ptmp,prr,CV_INTER_AREA);

        //cvtColor(frame,frame, CV_BGR2GRAY);
        imwrite("1.jpg",prrbak,vct);
        // 要读入整个文件，必须采用二进制打开
        filestr.open ("1.jpg", ios::binary);
        // 获取filestr对应buffer对象的指针
        pbuf=filestr.rdbuf();
        // 调用buffer对象方法获取文件大小
        size=pbuf->pubseekoff (0,ios::end,ios::in);
        pbuf->pubseekpos (0,ios::in);
        // 分配内存空间
        //buffer=new char[size];
        // 获取文件内容
        pbuf->sgetn (buffer,size);
        filestr.close();
        ss.str("");
        ss.clear();
        ss<<size;
        ss>>SIZE;
        std::cerr<<size<<' '<<SIZE<<"l\n";



        while(i<480)
        {
            FD_ZERO(&fdset);
            FD_SET(connfd,&fdset);
            maxfdp=connfd+1;
            switch(select(maxfdp,&fdset,&fdset,NULL,&timeout))
            {
            case -1:
                printf("selet error;");
                exit(-1);
            case 0:
                break;
            default:
                if(FD_ISSET(connfd,&fdset))
                {
                    writen(connfd, SIZE,10);
                    writen(connfd, buffer,size);
                    //delete[] buffer;
		    i=480;
                }
            }
        }

        waitKey(10);
    }//end while
}
int main(int argc, char *argv[])
{
    int listenfd,connfd;
    pid_t pid;
    socklen_t chilen;
    struct sockaddr_in cliaddr,servaddr;
    void sig_chld(int signo) ;


    if(!cap.isOpened())
    {
        printf("no camera!");
        return -1;
    }
    cap.set(CV_CAP_PROP_FRAME_WIDTH,640);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,480);

    if(!cap1.isOpened())
    {
        printf("no camera1!");
        return -1;
    }
    cap1.set(CV_CAP_PROP_FRAME_WIDTH,320);
    cap1.set(CV_CAP_PROP_FRAME_HEIGHT,240);

    if(!cap2.isOpened())
    {
         printf("no camera2!");
        return -1;
    }
    cap2.set(CV_CAP_PROP_FRAME_WIDTH,320);
    cap2.set(CV_CAP_PROP_FRAME_HEIGHT,240);

    listenfd=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET; // IPV4
    servaddr.sin_port=htons(portnumber); // (将本机器上的short数据转化为网络上的short数据)端口号
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY); // IP地址
    bind(listenfd,(sockaddr *)&servaddr,sizeof(sockaddr));
    listen(listenfd,10);
    signal(SIGCHLD,sig_chld);

    for(;;)
    {
        chilen=sizeof(cliaddr);
        if(   (connfd=accept(listenfd,(sockaddr *)&cliaddr,&chilen))<0  )
        {
            if(errno==EINTR)
                continue;
            else
                system("echo accept ");
        }
        if((pid=fork())==0)
        {
            close(listenfd);
            str_echo(connfd);
            close(connfd);
            exit(0);
        }
        close(connfd);
    }
}
void sig_chld(int signo)
{
    pid_t pid;
    int stat;
    while( (pid=waitpid(-1,&stat,WNOHANG)) >0  ) ;
    return;
}




