#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

#define MAXPACKETSIZE 10

int ChangeInterfaceStatus(int fd,bool flag)
{
	ifreq ifr;
	strncpy(ifr.ifr_name,"eth2",IFNAMSIZ-1);
	//get current status 
	if(ioctl(fd,SIOCGIFFLAGS,&ifr) != 0)
	{
	  perror("ioctl error");
	  return 1;
	}
	if (flag) ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
	else ifr.ifr_flags &= ~IFF_UP;

	return ioctl(fd,SIOCSIFFLAGS,&ifr);//0表示成功
}

int main(int argc, char *argv[])
{
	if(argc!=6)
	{
		printf("usage: %s destIP port packet_interval(us) output_file_name\n",argv[0]);
		exit(1);
	}
	int server_sockfd;
	int len;
	char *type;
	type=argv[1];
	struct in_addr server_addr;
	if(!inet_aton(argv[2], &server_addr)) 
		perror("inet_aton");
	int port=atoi(argv[3]);
	struct sockaddr_in remote_addr; 
	char buf[BUFSIZ];
	memset(&remote_addr,0,sizeof(remote_addr)); 
	remote_addr.sin_family=AF_INET;
	remote_addr.sin_addr=server_addr;
	// memcpy (&remote_addr.sin_addr, &server_addr, sizeof(server_addr));

	remote_addr.sin_port=htons(port); 

	FILE * fp;
	if ((fp=fopen("/home/guolab/output/statusRecord.txt","w+"))==NULL)
	{
		printf("Cannot open output_file\n");
		exit(1);
	}
	
	if((server_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket error");
		exit(1);
	}

	struct ifreq interface;
    strncpy(interface.ifr_ifrn.ifrn_name, "eth0", sizeof("eth0"));
    if (setsockopt(server_sockfd,SOL_SOCKET,SO_BINDTODEVICE,(char *)&interface,sizeof(interface))<0) 
    {
        perror("SO_BINDTODEVICE failed");
    }

	int nRecvBuf=1024*1024;
	setsockopt(server_sockfd,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
	int nSendBuf=1024*1024;
	setsockopt(server_sockfd,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));
	
	if(connect(server_sockfd,(const struct sockaddr *)&remote_addr,sizeof(remote_addr))<0)
	{
		perror("connect error");
		exit(1);
	}

	fprintf(fp,"connected to server  %s\n",inet_ntoa(remote_addr.sin_addr));
	fflush(fp);

	int times=atoi(argv[4]);
	int interval=atoi(argv[5]);
	int bytesToSend=MAXPACKETSIZE;
	char bufSent[MAXPACKETSIZE];

	int fd=socket(AF_INET,SOCK_DGRAM,0);//与内核通信的套接字

	if (recv(server_sockfd,buf,BUFSIZ,0)>0)
	{
		if (buf[0]=='r');
	}
	else
	{
		perror("rcvd ready mess error");
		exit(1);
	}

	for (int i=0;i<times;i++)
	{
		// down
		if (type[0]=='c')// // 不等server的grant,center
		{
			memset(bufSent,'.',bytesToSend*sizeof(bufSent[0]));
			// len=send(server_sockfd,"eth3 0",6,0);
			if (!ChangeInterfaceStatus(fd,false)) 
			{
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"eth2 down success------\n");
				fflush(fp);
			}
			else 
			{
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"eth2 down fail------\n");
				fflush(fp);
			}
			usleep(interval);
			// up
			memset(bufSent,'.',bytesToSend*sizeof(bufSent[0]));
			// len=send(server_sockfd,"eth3 1",6,0);
			if (!ChangeInterfaceStatus(fd,true)) 
			{
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"eth2 up success------\n");
				fflush(fp);
			}
			else 
			{
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"eth2 up fail------\n");
				fflush(fp);
			}
			usleep(interval);
		}	
		else if (type[0]=='b')//bgp,收到server的grant后更新interface
		{
			memset(bufSent,'.',bytesToSend*sizeof(bufSent[0]));
			len=send(server_sockfd,"eth3 0",6,0);
			if (recv(server_sockfd,buf,BUFSIZ,0)>0)
			{
				if (buf[0]=='r')
				{
					if (!ChangeInterfaceStatus(fd,false)) 
					{
						fseek(fp,0L,SEEK_END);
					    fprintf(fp,"eth2 down success------\n");
					    fflush(fp);
					}
					else 
					{
						fseek(fp,0L,SEEK_END);
					    fprintf(fp,"eth2 down fail------\n");
					    fflush(fp);
					}
				}
				else 
				{
					fseek(fp,0L,SEEK_END);
					fprintf(fp,"client recv fail------\n");
					fflush(fp);
					break;
				}
			}
			usleep(interval);
			// up
			memset(bufSent,'.',bytesToSend*sizeof(bufSent[0]));
			len=send(server_sockfd,"eth3 1",6,0);
			if (recv(server_sockfd,buf,BUFSIZ,0)>0)
			{
				if (buf[0]=='r')
				{
					if (!ChangeInterfaceStatus(fd,true)) 
					{
						fseek(fp,0L,SEEK_END);
					    fprintf(fp,"eth2 up success------\n");
					    fflush(fp);
					}
					else 
					{
						fseek(fp,0L,SEEK_END);
					    fprintf(fp,"eth2 up fail------\n");
					    fflush(fp);
					}
				}
				else
				{
					fseek(fp,0L,SEEK_END);
					fprintf(fp,"client recv fail------\n");
					fflush(fp);
					break;
				}
			}
			usleep(interval);
		}		
	}
	close(server_sockfd);
    return 0;
}
