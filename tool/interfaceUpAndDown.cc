#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <iomanip>
// 方法二
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

using namespace std;

int main(int argc, char const *argv[])
{
	/*struct timeval tv;
	gettimeofday(&tv,NULL);
	FILE * fpB;
	if ((fpB=fopen("/home/guolab/output/interfacesChangeTimeRecord.txt","a+"))==NULL)
	{
		printf("Cannot open interfacesChangeTimeRecord.txt\n");
		exit(0);
	}

	fseek(fpB,0L,SEEK_END);
    fprintf(fpB, "%f\n",tv.tv_sec+tv.tv_usec*0.000001);//关闭或者打开网卡
    fflush(fpB);
    fclose(fpB);
    
	stringstream suffix;
	char buff[3];
	FILE * fpA;
	if (fpA=popen("ifconfig eth0 | grep \"inet addr:\" | awk '{print $2}' | cut -c 13-","r"))//截取eth0 ip的后两位
	{
		fgets(buff,sizeof(buff),fpA);
		//cout << "buff:" << buff << endl;
	}
	suffix << buff;

	//sleep(1);//挂起,是希望能先让pssh执行完,别master的命令还没到node，BGP的update就到了
	if (strcmp(argv[1],suffix.str().c_str())==0)
	{
		string command,commandA,commandB;
        commandA=argv[3];
        commandB=argv[4];
        command=commandA+" "+commandB;
	    system(command.c_str());
	}
	else if (strcmp(argv[2],suffix.str().c_str())==0)
	{
		string command,commandA,commandB;
        commandA=argv[5];
        commandB=argv[6];
        command=commandA+" "+commandB;
	    system(command.c_str());
	}*/

	int fd;
	struct timeval begin,end;
  	ifreq ifr;
  	fd = socket(AF_INET, SOCK_DGRAM, 0);
  	// strncpy(ifr.ifr_name,"eth5",IFNAMSIZ-1);
  	strncpy(ifr.ifr_name,argv[1],IFNAMSIZ-1);

  	//get current status	
  	if(ioctl(fd,SIOCGIFFLAGS,&ifr) != 0)
  	{
    	perror("ioctl");
    	return 1;
  	}

  	if (*argv[2]=='1')//let net work up
  	{
      printf("ifr.ifr_flags=%hd\n",ifr.ifr_flags);
  		ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
      printf("ifr.ifr_flags=%hd\n",ifr.ifr_flags);
  	}
  	else if (*argv[2]=='0')//let net work down
  	{
      printf("ifr.ifr_flags=%hd\n",ifr.ifr_flags);
  		ifr.ifr_flags &= ~IFF_UP;
      printf("ifr.ifr_flags=%hd\n",ifr.ifr_flags);
  	}
  	//change status
  	gettimeofday(&begin,NULL);
  	if(ioctl(fd, SIOCSIFFLAGS, &ifr)!=0)
  	{
    	perror("ioctl");
    	return 1;
  	}
  	gettimeofday(&end,NULL);
  	cout << setiosflags(ios::fixed) << setprecision(9) << begin.tv_sec+begin.tv_usec*0.000001 << " " << end.tv_sec+end.tv_usec*0.000001 << endl;
  	close(fd);

    return 0;
}