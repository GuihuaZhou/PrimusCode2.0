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
#include <net/if.h>
#include <pthread.h>

FILE * fp;

void* server_thread_func(void* client_sockfd)
{
  int sock=*(int*)client_sockfd;
  int len;
  char buf[BUFSIZ]={};
  int keepaliveNum;

  int fd=socket(AF_INET,SOCK_DGRAM,0);//与内核通信的套接字
  ifreq ifrA,ifrB;
  strncpy(ifrA.ifr_name,"eth3",IFNAMSIZ-1);
  strncpy(ifrB.ifr_name,"eth4",IFNAMSIZ-1);

  //get current status 
  if(ioctl(fd,SIOCGIFFLAGS,&ifrA) != 0)
  {
	   	fseek(fp,0L,SEEK_END);
		fprintf(fp,"ioctl ifrA error\n");
		fflush(fp);
	   	exit(1);
  }

  if(ioctl(fd,SIOCGIFFLAGS,&ifrB) != 0)
  {
	   	fseek(fp,0L,SEEK_END);
		fprintf(fp,"ioctl ifrB error\n");
		fflush(fp);
	   	exit(1);
  }

  while(1)
  {
    if((len=recv(sock,buf,BUFSIZ,0))<=0)
    {
      	fseek(fp,0L,SEEK_END);
		fprintf(fp,"recv error\n");
		fflush(fp);
     	break;// exit(1);
    }
   //  if (buf[3]=='3')//改变eth3
   //  {
   //  	if (buf[5]=='0') ifrA.ifr_flags &= ~IFF_UP;
   //  	else if (buf[5]=='1') ifrA.ifr_flags |= IFF_UP | IFF_RUNNING;
   //  	if (ioctl(fd,SIOCSIFFLAGS,&ifrA)==0)//0表示成功
	  //   {
	  //   	fseek(fp,0L,SEEK_END);
			// fprintf(fp,"change eth3 success------\n");
			// fflush(fp);
	  //   }
	  //   else 
	  //   {
	  //   	fseek(fp,0L,SEEK_END);
	  //   	fprintf(fp,"change eth3 fail------\n");
	  //   	fflush(fp);
	  //   	// exit(1);
	  //   }
   //  }
   //  else if (buf[3]=='4')//改变eth4
   //  {
   //  	if (buf[5]=='0') ifrB.ifr_flags &= ~IFF_UP;
   //  	else if (buf[5]=='1') ifrB.ifr_flags |= IFF_UP | IFF_RUNNING;
   //  	if (ioctl(fd,SIOCSIFFLAGS,&ifrB)==0)//0表示成功
	  //   {
	  //   	fseek(fp,0L,SEEK_END);
			// fprintf(fp,"change eth4 success------\n");
			// fflush(fp);
	  //   }
	  //   else 
	  //   {
	  //   	fseek(fp,0L,SEEK_END);
	  //   	fprintf(fp,"change eth4 fail------\n");
	  //   	fflush(fp);
	  //   	// exit(1);
	  //   }
   //  }
    if (buf[3]=='3')//改变eth3
    {
    	if (buf[5]=='0')
    	{
    		ifrA.ifr_flags &= ~IFF_UP;
    		if (ioctl(fd,SIOCSIFFLAGS,&ifrA)==0)//0表示成功
		    {
		    	fseek(fp,0L,SEEK_END);
				fprintf(fp,"down eth3 success------\n");
				fflush(fp);
		    }
		    else 
		    {
		    	fseek(fp,0L,SEEK_END);
		    	fprintf(fp,"down eth3 fail------\n");
		    	fflush(fp);
		    	// exit(1);
		    }
    	} 
    	else if (buf[5]=='1')
    	{
    		ifrA.ifr_flags |= IFF_UP | IFF_RUNNING;
    		if (ioctl(fd,SIOCSIFFLAGS,&ifrA)==0)//0表示成功
		    {
		    	fseek(fp,0L,SEEK_END);
				fprintf(fp,"up eth3 success------\n");
				fflush(fp);
		    }
		    else 
		    {
		    	fseek(fp,0L,SEEK_END);
		    	fprintf(fp,"up eth3 fail------\n");
		    	fflush(fp);
		    	// exit(1);
		    }
    	} 
    }
    else if (buf[3]=='4')//改变eth4
    {
    	if (buf[5]=='0')
    	{
    		ifrB.ifr_flags &= ~IFF_UP;
    		if (ioctl(fd,SIOCSIFFLAGS,&ifrB)==0)//0表示成功
		    {
		    	fseek(fp,0L,SEEK_END);
				fprintf(fp,"down eth4 success------\n");
				fflush(fp);
		    }
		    else 
		    {
		    	fseek(fp,0L,SEEK_END);
		    	fprintf(fp,"down eth4 fail------\n");
		    	fflush(fp);
		    	// exit(1);
		    }
    	} 
    	else if (buf[5]=='1') 
    	{
    		ifrB.ifr_flags |= IFF_UP | IFF_RUNNING;
	    	if (ioctl(fd,SIOCSIFFLAGS,&ifrB)==0)//0表示成功
		    {
		    	fseek(fp,0L,SEEK_END);
				fprintf(fp,"up eth4 success------\n");
				fflush(fp);
		    }
		    else 
		    {
		    	fseek(fp,0L,SEEK_END);
		    	fprintf(fp,"up eth4 fail------\n");
		    	fflush(fp);
		    	// exit(1);
		    }
		}
    }
    // send grant
	if (send(sock,"ready",5,0)<=0) 
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"send message errorr\n");
		fflush(fp);
		exit(1);
	}
   }
}

int main(int argc, char *argv[])
{
	pthread_t server_thread;
	if(argc!=2)
	{
		printf("usage: %s port checkThroughInterval output_file_name1 output_file_name1\n",argv[0]);
		exit(1);
	}

	if ((fp=fopen("/home/guolab/output/statusRecord.txt","w+"))==NULL)
	{
		printf("Cannot open output_file\n");
		exit(1);
	}

	int port=atoi(argv[1]);
	int server_sockfd;
	int len;
	struct sockaddr_in my_addr;  
	struct sockaddr_in remote_addr; 
	char buf[BUFSIZ];  
	memset(&my_addr,0,sizeof(my_addr)); 
	my_addr.sin_family=AF_INET; 
	my_addr.sin_addr.s_addr=INADDR_ANY;
	my_addr.sin_port=htons(port); 
	//Socket pointer for client 
	
	if((server_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{  
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"socket error\n");
		fflush(fp);
		exit(1);
	}

	int on=100;  
    if((setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)  
    {  
    	fseek(fp,0L,SEEK_END);
		fprintf(fp,"setsockopt failed\n");
		fflush(fp);
        exit(1);
    }  

	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"bind error\n");
		fflush(fp);
		exit(1);
	}

	struct ifreq interface;
    strncpy(interface.ifr_ifrn.ifrn_name, "eth0", sizeof("eth0"));
    if (setsockopt(server_sockfd,SOL_SOCKET,SO_BINDTODEVICE,(char *)&interface,sizeof(interface))<0) 
    {
        perror("SO_BINDTODEVICE failed");
    }
	
	if (listen(server_sockfd,100)==-1)
	{
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"listen error\n");
		fflush(fp);
		exit(1);
	}
	socklen_t size=sizeof(struct sockaddr_in);
	
	while (1)
	{
		int* client_sockfd=(int*)malloc(sizeof(int));
		int value;

		int nRecvBuf=1024*1024;
		setsockopt(*client_sockfd,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
		//发送缓冲区
	    int nSendBuf=1024*1024;//设置为1M
	    setsockopt(*client_sockfd,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));

		if ((value=accept(server_sockfd,(struct sockaddr *)&remote_addr,&size))==-1)
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"accept error\n");
			fflush(fp);
			exit(1);
		}
		
		// send grant
		if (send(value,"ready",5,0)<=0) 
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"send grant errorr\n");
			fflush(fp);
		  	exit(1);
		}
		else 
		{
			fseek(fp,0L,SEEK_END);
		    fprintf(fp,"send grant success------\n");
		    fflush(fp);
		}

		*client_sockfd=value;
		if (pthread_create(&server_thread,NULL,server_thread_func,(void*)client_sockfd)<0)
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"could not create thread\n");
			fflush(fp);
      		exit(1);
		}
	}
	close(server_sockfd);
    return 0;
}
