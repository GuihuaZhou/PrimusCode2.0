#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	//printf("hello udp-server\n");
	if(argc!=5)
	{
		printf("usage: %s flow_size(bytes) results_file_name throughput_file_name\n",argv[0]);
		exit(1);
	}
	//int flow_size=atoi(argv[1]);
	double checkThroughInterval=atof(argv[2]);

	int server_sockfd;//·þÎñÆ÷¶ËÌ×½Ó×Ö
	int len;
	struct sockaddr_in my_addr;   //·þÎñÆ÷ÍøÂçµØÖ·½á¹¹Ìå
	struct sockaddr_in remote_addr; //¿Í»§¶ËÍøÂçµØÖ·½á¹¹Ìå
	int sin_size;
	char buf[BUFSIZ];  //Êý¾Ý´«ËÍµÄ»º³åÇø
	memset(&my_addr,0,sizeof(my_addr)); //Êý¾Ý³õÊ¼»¯--ÇåÁã
	my_addr.sin_family=AF_INET; //ÉèÖÃÎªIPÍ¨ÐÅ
	my_addr.sin_addr.s_addr=INADDR_ANY;//·þÎñÆ÷IPµØÖ·--ÔÊÐíÁ¬½Óµ½ËùÓÐ±¾µØµØÖ·ÉÏ
	my_addr.sin_port=htons(atoi(argv[1])); //·þÎñÆ÷¶Ë¿ÚºÅ
	
	FILE * fp;
	if ((fp = fopen(argv[3], "w")) == NULL)
	{
		printf("Cannot open output_file %s\n",argv[3]);
		exit(0);
	}

	FILE * fp1;
	if ((fp1 = fopen(argv[4], "w")) == NULL)
	{
		printf("Cannot open output_file %s\n",argv[4]);
		exit(0);
	}

	/*´´½¨·þÎñÆ÷¶ËÌ×½Ó×Ö--IPv4Ð­Òé£¬ÃæÏòÁ¬½ÓÍ¨ÐÅ£¬TCPÐ­Òé*/
	if((server_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{  
		perror("socket");
		return 1;
	}
	
        /*½«Ì×½Ó×Ö°ó¶¨µ½·þÎñÆ÷µÄÍøÂçµØÖ·ÉÏ*/
	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(my_addr))<0)
	{
		perror("bind");
		return 1;
	}
	
	/*½ÓÊÕ¿Í»§¶ËµÄÊý¾Ý²¢½«Æä·¢ËÍ¸ø¿Í»§¶Ë--recv·µ»Ø½ÓÊÕµ½µÄ×Ö½ÚÊý£¬send·µ»Ø·¢ËÍµÄ×Ö½ÚÊý*/
	int receiveCount=0;
	int receiveBytes=0;
	int receiveBytesInterval=0;
	int receivedRatio=1;  ///received x/10

	struct timeval start,time;
	gettimeofday( &start, NULL );
	double stopTime=start.tv_sec+50.0;

	double lastPacketTime;
	double thisPacketTime;
	double maxTimeDiffTwoPackets=-1;
	double timeDiffTwoPackets=0;
	
	//int totalPackets=(int) flow_size/1448+1;
	printf("waiting for a packet...\n"); 
	fflush(stdout);
	fseek(fp,0L,SEEK_CUR);
	long int curPosition=ftell(fp);
	//struct timeval start;
	//fprintf(fp,"IP Address:%s\n",inet_ntoa(remote_addr.sin_addr));
	fseek(fp,0L,SEEK_END);
	fprintf(fp, "ReceTime(s)\tThroughput Rate(Mbps)\n");
	fflush(fp);
	
	while(1)
	{
		unsigned int addrLen = sizeof(struct sockaddr_in);
		if((len = recvfrom(server_sockfd,buf,BUFSIZ,0,(struct sockaddr*)&remote_addr,&addrLen))<0)
		{
		 	printf("recvfrom().\r\n");
			exit(1);
		}

		receiveCount++;
		receiveBytesInterval+=len;
		receiveBytes=receiveBytes+len;

		if(receiveCount==1)
		{
			gettimeofday( &time, NULL );
			lastPacketTime=time.tv_sec+time.tv_usec*0.000001;
			fseek(fp1,0L,SEEK_END);
			//fprintf(fp1, "stopTime:%lfs and checkThroughInterval:%lfs\n",stopTime,checkThroughInterval);
		    fprintf(fp1,"[%lf s]:\treceived %d bytes....\n",lastPacketTime,len);
		    fflush(fp1);
			continue;			
		}

		//struct timeval end;
		//gettimeofday( &end, NULL );
		//double timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec; 
		//double timeuse = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec)*0.000001; 
		//fprintf(fp1,"%lf\t%d\n",timeuse,len);
		//fflush(fp1);
		/*if(maxTimeDiffTwoPackets==-1)
		{
			maxTimeDiffTwoPackets=0;
			continue;
		}*/

		gettimeofday( &time, NULL );
		if (time.tv_sec>stopTime) break;
		thisPacketTime=time.tv_sec+time.tv_usec*0.000001;
		timeDiffTwoPackets=thisPacketTime-lastPacketTime;

		fseek(fp1,0L,SEEK_END);
		fprintf(fp1,"[%lf s]:\treceived %d bytes....\n",thisPacketTime,len);
		fflush(fp1);

		if (timeDiffTwoPackets>checkThroughInterval*10)
		{
			for (lastPacketTime+=checkThroughInterval;lastPacketTime<=thisPacketTime-checkThroughInterval;)
			{
				fseek(fp,0L,SEEK_END);
		        /*fprintf(fp, "%lf\t0\n",lastPacketTime);*/
		        fprintf(fp, "%lf\t%lf\n",lastPacketTime,(double)(receiveBytesInterval*8/(timeDiffTwoPackets*1024*1024)));
		        fflush(fp);
		        lastPacketTime+=checkThroughInterval;
			}
			fseek(fp,0L,SEEK_END);
		    fprintf(fp, "%lf\t%lf\n",thisPacketTime,(double)(receiveBytesInterval*8/(timeDiffTwoPackets*1024*1024)));
		    fflush(fp);
		    lastPacketTime=thisPacketTime;
		    receiveBytesInterval=0;
		}
		else
		{
            if (timeDiffTwoPackets>=checkThroughInterval && timeDiffTwoPackets<=checkThroughInterval*10)
            {
                fseek(fp,0L,SEEK_END);
		        fprintf(fp, "%lf\t%lf\n",thisPacketTime,(double)(receiveBytesInterval*8/(timeDiffTwoPackets*1024*1024)));
		        fflush(fp);
		        lastPacketTime=thisPacketTime;
		        receiveBytesInterval=0;
            }
		}

		/*if(timeDiffTwoPackets>=maxTimeDiffTwoPackets)
		{
			maxTimeDiffTwoPackets=timeDiffTwoPackets;		
		}*/
		//fseek(fp,curPosition,SEEK_SET);
		/*fprintf(fp,"Total %d bytes, %d packets sent to the server!\n",receiveBytes,receiveCount);
		fprintf(fp,"%d bytes, %d packets lost. Max reconnection time is %lf(s)!\n",flow_size-receiveBytes, totalPackets-receiveCount,maxTimeDiffTwoPackets);
		fflush(fp);*/
		
		/*if (receiveBytes>=flow_size)
		{
			break;
		}*/
	}
	fseek(fp,0L,SEEK_END);
	fprintf(fp,"\n\nTotal %d bytes, %d packets received!\n",receiveBytes,receiveCount);
	//fprintf(fp,"%d bytes and %d packets lost. Max reconnection time is %lf(s)!\n",flow_size-receiveBytes, totalPackets-receiveCount,maxTimeDiffTwoPackets);
	fflush(fp);
	fclose(fp);
	fclose(fp1);
	close(server_sockfd);
    return 0;
}
