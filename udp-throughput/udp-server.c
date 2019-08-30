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
	if(argc!=2)
	{
		printf("usage: %s flow_size(bytes) results_file_name throughput_file_name\n",argv[0]);
		exit(1);
	}

	int server_sockfd;
	int len;
	struct sockaddr_in my_addr;  
	struct sockaddr_in remote_addr; 
	int sin_size;
	char buf[BUFSIZ]; 
	memset(&my_addr,0,sizeof(my_addr)); 
	my_addr.sin_family=AF_INET; 
	my_addr.sin_addr.s_addr=INADDR_ANY;
	my_addr.sin_port=htons(atoi(argv[1])); 
	
	// FILE * fpA;
	// if ((fpA = fopen("/home/guolab/output/throughputRatio.txt", "w")) == NULL)
	// {
	// 	printf("Cannot open throughputRatio file\n");
	// 	exit(0);
	// }

	FILE * fpB;
	if ((fpB = fopen("/home/guolab/output/serverRecord.txt", "w")) == NULL)
	{
		printf("Cannot open packet record file\n");
		exit(0);
	}

	if((server_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{  
		perror("socket");
		return 1;
	}
	
	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(my_addr))<0)
	{
		perror("bind");
		return 1;
	}
	
	int receiveCount=0;
	int receiveBytes=0;
	int receivedRatio=1; 
	double maxTimeDiffTwoPackets=-1;
	double timeDiffTwoPackets=0;
	// printf("waiting for a packet...\n"); 
	// fflush(stdout);
	// fseek(fpA,0L,SEEK_CUR);
	// long int curPosition=ftell(fpA);
	struct timespec lastStamp,thisPacketTime;
	double interval=500;//检测吞吐率间隔，us
	int allReceiveBytes=0,tempReceiveBytes=0;
	double timeuse=0;
	double throughputRatio=0;
	
	while(1)
	{
		unsigned int addrLen = sizeof(struct sockaddr_in);
		if((len = recvfrom(server_sockfd,buf,BUFSIZ,0,(struct sockaddr*)&remote_addr,&addrLen))<0)
		{
		 	// fprintf(fpA,"recvfrom() error.\r\n");
			// break;
		}

		allReceiveBytes+=len;
		clock_gettime(CLOCK_MONOTONIC,&thisPacketTime);
		fseek(fpB,0L,SEEK_END);
		fprintf(fpB,"%d\t%f\n",len,thisPacketTime.tv_sec+thisPacketTime.tv_nsec*0.000000001);//每收到一个packet都要记录
		fflush(fpB);

		// tempReceiveBytes+=len;//一次检测间隔内收到的字节数
		// receiveCount++;

		// if(receiveCount==0) 
		// {
		// 	clock_gettime(CLOCK_MONOTONIC,&lastStamp);
		// 	fseek(fpB,0L,SEEK_END);
		// 	fprintf(fpB,"%d\t%.9f\n",len,lastStamp.tv_sec+lastStamp.tv_nsec*0.000000001);//每收到一个packet都要记录
		// 	fflush(fpB);
		// 	continue;
		// }

		// clock_gettime(CLOCK_MONOTONIC,&thisPacketTime);
		// timeuse=1000000*(thisPacketTime.tv_sec-lastStamp.tv_sec)+(thisPacketTime.tv_nsec-lastStamp.tv_nsec)*0.001; 

		// fseek(fpB,0L,SEEK_END);
		// fprintf(fpB,"%d\t%.9f\n",len,thisPacketTime.tv_sec+thisPacketTime.tv_nsec*0.000000001);//每收到一个packet都要记录
		// fflush(fpB);

		// if (timeuse>=interval)//大于间隔则计算吞吐率
		// {
		// 	throughputRatio=(tempReceiveBytes*8)/timeuse;
		// 	fseek(fpA,0L,SEEK_END);
		// 	fprintf(fpA,"%f\n",throughputRatio);
		// 	allReceiveBytes+=tempReceiveBytes;
		// 	tempReceiveBytes=0;
		// 	lastStamp=thisPacketTime;
		// 	if (timeuse>maxTimeDiffTwoPackets) maxTimeDiffTwoPackets=timeuse;
		// }
		// if (allReceiveBytes>=flow_size) break;
		// else//
		// {
		// 	// 
		// }
		// if(maxTimeDiffTwoPackets==-1)
		// {
		// 	gettimeofday( &lastPacketTime, NULL );
		// 	receiveCount++;
		// 	receiveBytes=receiveBytes+len;
		// 	maxTimeDiffTwoPackets=0;
		// 	continue;
		// }
		// gettimeofday( &thisPacketTime, NULL );
		// timeDiffTwoPackets = 1000000 * ( thisPacketTime.tv_sec - lastPacketTime.tv_sec ) + thisPacketTime.tv_usec - lastPacketTime.tv_usec;
		// lastPacketTime = thisPacketTime;
		// if(timeDiffTwoPackets>=maxTimeDiffTwoPackets)
		// {
		// 	maxTimeDiffTwoPackets=timeDiffTwoPackets;		
		// }
		// fseek(fpA,curPosition,SEEK_SET);
		// //printf("%d bytes, %d packets received. Max reconnection time is %lf(us)!\n",receiveBytes, receiveCount, maxTimeDiffTwoPackets);
		// fprintf(fpA,"Total %d bytes, %d packets sent to the server!\n",receiveBytes,receiveCount);
		// fprintf(fpA,"%d bytes, %d packets lost. Max reconnection time is %lf!\n",flow_size-receiveBytes, totalPackets-receiveCount,maxTimeDiffTwoPackets);
		// fflush(fpA);
		// receiveCount++;
		// receiveBytes=receiveBytes+len;
		// if(receiveBytes>=(flow_size*((float) receivedRatio)/10))
		// {
		// 	printf("[%f s]: %d/10 done, received %d bytes....\n",timeuse/1000000,receivedRatio,receiveBytes);
		// 	receivedRatio++;
		// }
		// if (receiveBytes>=flow_size)
		// {
		// 	break;
		// }
	}
	// fseek(fpB,0L,SEEK_END);
	// fprintf(fpB,"Total %d bytes, %d packets sent to the server!\n",allReceiveBytes,receiveCount);
	// fprintf(fpB,"%f bytes, %d packets lost. Max reconnection time is %lf!\n",flow_size-allReceiveBytes,totalPackets-receiveCount,maxTimeDiffTwoPackets);
	// fflush(fpB);
	// // fclose(fpA);
	// fclose(fpB);
	
	close(server_sockfd);
    return 0;
}
