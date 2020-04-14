#include <stdio.h>
#include <string.h>
#include <iostream>
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

using namespace std;

int main(int argc, char const *argv[])
{
	int clientNum=atoi(argv[1]);
	int linkStatus[]={1,1,1,1,1,1,1,1};
	int spineMGMTStatus[]={1,1,1,1,1,1,1,1};
	int leafMGMTStatus[]={1,1};
	int spineSock[]={-1,-1,-1,-1,-1,-1,-1,-1};
	int leafSock[]={-1,-1};
	int routeType=atoi(argv[2]);// 路由协议类型，1=BGP，2=Primus
	string spineIP[]={"10.0.80.30","10.0.80.31","10.0.80.32","10.0.80.33","10.0.80.34","10.0.80.35","10.0.80.36","10.0.80.37"};
	string leafIP[]={"10.0.80.20","10.0.80.21"};

	struct sockaddr serverAddr;
	memset(&serverAddr,0,sizeof(serverAddr)); //数据初始化--清零
  serverAddr.sin_family=AF_INET; //设置为IP通信
  serverAddr.sin_addr.s_addr=INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
  serverAddr.sin_port=htons(atoi(argv[3]));

  int serverSock;
  if((serverSock=socket(PF_INET,SOCK_STREAM,0))<0)
  {
    // 
  }
  int value=1;
  if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEPORT,&value,sizeof(value))<0)
  {
    // Logfout << GetNow() << "TCPRoute set SO_REUSEPORT error" << endl;
    // exit(0);
  }
  if (setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
  {
    // Logfout << GetNow() << "TCPRoute set SO_REUSEADDR error" << endl;
    // exit(0);
  }
  if(bind(serverSock,(struct sockaddr *)&serverAddr,sizeof(struct sockaddr))<0)
  {
    Logfout << GetNow() << "TCPRoute Bind Socket Failed." << endl;
    exit(0);
  }
  listen(serverSock,1024);

  int connectClientNum=0;
  while (connectClientNum<clientNum)
  {
  	struct sockaddr_in clientAddr;
    unsigned sin_size=sizeof(struct sockaddr_in);
    /*等待客户端连接请求到达*/
    if ((clientSock=accept(serverSock,(struct sockaddr *)&(clientAddr),&sin_size))==-1)
    {
      // Logfout << GetNow() << "Sock(" << clientSock << ") accept socket error:" << strerror(errno) << " (errno:" << errno <<  ")." << endl;
      // continue;
    }
    connectClientNum++;

    int nRecvBuf=TCP_BUF_SIZE*1024;
    setsockopt(clientSock,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(int));
    //发送缓冲区
    int nSendBuf=TCP_BUF_SIZE*1024;
    setsockopt(clientSock,SOL_SOCKET,SO_SNDBUF,&nSendBuf,sizeof(int));

    for (int i=0;i<spineIP.size();i++)
    {
    	if (!strcmp(spineIP[i],inet_ntoa(clientAddr.sin_addr)))// 地址相同
    	{
    		spineSock[i]=clientSock;
    		break;
    	}
    }
    for (int i=0;i<leafIP.size();i++)
    {
    	if (!strcmp(leafIP[i],inet_ntoa(clientAddr.sin_addr)))
    	{
    		leafSock[i]=clientSock;
    		break;
    	}
    }
  }

  while ()
  {
  	// 
  }

	return 0;
}