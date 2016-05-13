#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_ntoa()函数的头文件
#include <unistd.h>
#include <errno.h>
#include <opencv2/opencv.hpp>
#include <getopt.h>
#include <stdio.h>

using namespace cv;

#define portnumber 3333 //定义端口号：（0-1024为保留端口号，最好不要用）
std::stringstream ss;
static int COLS=960,ROWS=480;//width,heigh


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

void str_quest(int sockfd)
{
    int maxfdp;
    fd_set fdset;
    timeval timeout={3,0};
    
    
    long  size;
    char  SIZE[10];
    FILE *outfile; 
    char *recvline=new char[120000];
    
    IplImage *psrc,*pdst;
    Size sz;
    sz.width = COLS;
    sz.height = ROWS;
    
    
    
    while(1)
    {
        //int i=0;
        //while(i<480)
        //{
        FD_ZERO(&fdset);
        FD_SET(sockfd,&fdset);
        maxfdp=sockfd+1;
        switch(select(maxfdp,&fdset,&fdset,NULL,&timeout))
        {
        case 0:
            break;
        case -1:
            printf("selet error;");
            exit(-1);
        default:
            if(FD_ISSET(sockfd,&fdset))
            {
                readn(sockfd,SIZE,10);
                ss.str("");
                ss.clear();
                ss<<SIZE;
                ss>>size;
                std::cerr<<size<<"\n";        
                //recvline=new char[size];
                readn(sockfd,recvline,size);
                outfile = fopen("1.jpg","wb");
                fwrite((uchar *)recvline,sizeof(uchar),size,outfile );
                fclose(outfile);
                //          i=480;
                //delete[] recvline;
            }
            
        }
        //}

        psrc = cvLoadImage("1.jpg",CV_LOAD_IMAGE_COLOR); 
        pdst = cvCreateImage(sz,psrc->depth,psrc->nChannels);          
        Mat im(pdst);
        cvResize(psrc,pdst,CV_INTER_AREA);  
        imshow("image",im);
        waitKey(1);
	cvReleaseImage(&psrc);
        cvReleaseImage(&pdst);            
        
    }
}
static void usage(FILE *fp,char ** argv){
    fprintf(fp,"Usage:%s[options]\n\n"
            "Options:\n"
            "-i | --ip          Ip of client\n"
            "-h | --help        Print this message\n"
            "-r | --rows        Set image's rows\n"
            "-c | --cols        Set image's cols\n"
            "", argv[0]);
}

static const char short_options[] = "i:hr:c:";

static const struct option long_options[] = { {"ip",required_argument,
                                              NULL,'i'},{"help",no_argument,NULL,'h'},{"rows",
                                                                                       required_argument,NULL,'r'},{"cols",
                                                                                                                    required_argument,NULL,'c'},
{0,0,0,0} };

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in servaddr;
    char *ipname = "192.168.8.103";
    while(1)
    {
        int index;
        int c;
        c = getopt_long(argc, argv, short_options, long_options, &index);
        if(-1 == c)
            break;
        switch(c){
        case 0:
            break;
        case 'i':
            ipname = optarg;
            break;
        case 'h':
            usage(stdout,argv);
            exit(EXIT_FAILURE);
        case 'r':
            ss.str("");
            ss.clear();
            ss<<optarg;
            ss>>ROWS;
            printf("Set ROWS = %d\n",ROWS);
            break;
        case 'c':
            ss.str("");
            ss.clear();
            ss<<optarg;
            ss>>COLS;
            printf("Set COLS = %d\n",COLS);
            break;
        default:
            usage(stderr,argv);
            exit(EXIT_FAILURE);
        }
    }
    //if(argc!=2)
    //{
    //    fprintf(stderr,"Usage:%s hostname \a\n",argv[0]);
    //    exit(1);
    //}
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET; // IPV4
    servaddr.sin_port=htons(portnumber); // (将本机器上的short数据转化为网络上的short数据)端口号
    struct hostent *host=gethostbyname(ipname);
    servaddr.sin_addr=*((struct in_addr *)host->h_addr); // IP地址
    if(connect(sockfd,(struct sockaddr *)(&servaddr),sizeof(servaddr))==-1)
    {
        fprintf(stderr,"Connect Error:%s\a\n",strerror(errno));
        exit(1);
    }
    str_quest(sockfd);
    
    exit(0);
}

