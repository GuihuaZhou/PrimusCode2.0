#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <stddef.h>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <pthread.h>
#include <time.h> 
#include <sys/time.h> 
#include <sys/types.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <ifaddrs.h>  
#include <unistd.h>
#include <net/if.h> 
#include <net/route.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <iomanip>
#include <strings.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include "/usr/include/net/if_arp.h"

#define MGMT_INTERFACE "eth0"

using namespace std;

struct ident
{
	int level;
	int position;
};

int m_SpineNodes=16;
int m_LeafNodes=4;
int m_ToRNodes=2;
int m_nPods=2;
int m_nMaster=1;
double P1=0.1;
double P2=0.1;
ident m_Node;

ident GetNextHopIdent(int level,int position,int interface,int * nextHopInterface)//interface从2开始,position从0开始
{
  ident nextHopIdent;
  if (level==1)//若是第一层的端口，则返回它的下一跳，即第二层的下行链路端口的位置信息
  {
    nextHopIdent.level=2;
    nextHopIdent.position=position/m_ToRNodes*m_LeafNodes+interface-1;
    *nextHopInterface=position%m_ToRNodes+(m_SpineNodes/m_LeafNodes)+1;
  }
  else if (level==2)
  {
    if (interface<((m_SpineNodes/m_LeafNodes)+1))//若是第二层的上行链路端口，则返回它的下一跳，即第三层的下行链路端口的位置信息
    {
      nextHopIdent.level=3;
      nextHopIdent.position=position%m_LeafNodes*(m_SpineNodes/m_LeafNodes)+interface-1;
      *nextHopInterface=position/m_LeafNodes+1;
    }
    else if (interface>=((m_SpineNodes/m_LeafNodes)+1))//若是第二层的下行链路端口，则返回它的下一跳，即第一层的上行链路端口的位置信息
    {
      nextHopIdent.level=1;
      nextHopIdent.position=position/m_LeafNodes*m_ToRNodes+interface-(m_SpineNodes/m_LeafNodes)-1;
      *nextHopInterface=position%m_LeafNodes+1;
    }
  }
  else if (level==3)//若是第三层的下行链路端口，则返回它的下一跳，即第二层的上行链路端口的位置信息
  {
    nextHopIdent.level=2;
    nextHopIdent.position=position/(m_SpineNodes/m_LeafNodes)+(interface-1)*m_LeafNodes;
    *nextHopInterface=position%(m_SpineNodes/m_LeafNodes)+1;
  }
  return nextHopIdent;
}

int factorial(int num)
{
	if (num==0) return 1;
	else return num*factorial(num-1);
}

double Probability(ident temp,int fromInterface)
{
	int nNeighbor;
	// cout << temp.level << "." << temp.position << endl;
	if (temp.level==1 && fromInterface!=-1) 
	{
		// cout << temp.level << "." << temp.position << "'s probability is " << pow(P2,m_nMaster) << endl;
		return pow(P2,m_nMaster);
	}
	else 
	{
		double probability,probabilityA,probabilityB,probabilityC;
		probability=0;
		probabilityA=pow(P2,m_nMaster);
		if (temp.level==1) nNeighbor=m_LeafNodes;
		else if (temp.level==2) nNeighbor=m_ToRNodes+m_SpineNodes/m_LeafNodes;
		else if (temp.level==3) nNeighbor=m_nPods;
		if (fromInterface!=-1) nNeighbor-=1;
		probabilityB=1;
		probabilityC=0;
		for (int i=0;i<nNeighbor;i++)//i表示好的直连链路数
		{
			int j=1;
			if (temp.level==2)//
			{
				//不在同一个Pod内
				if (m_Node.position/m_ToRNodes!=temp.position/m_LeafNodes) j=m_SpineNodes/m_LeafNodes+1;
			}
			for (;j<=i;j++)
			{
				if (j==fromInterface) continue;
				int nextHopInterface=0;
				ident nextIdent=GetNextHopIdent(temp.level,temp.position,j,&nextHopInterface);
				cout << nextIdent.level << "." << nextIdent.position << endl;
				probabilityB*=Probability(nextIdent,nextHopInterface);
			}
			if (fromInterface!=-1) probabilityC+=(factorial(nNeighbor-1)/(factorial(i)*factorial(nNeighbor-i-1)))*pow(P1,nNeighbor-i-1)*pow(1-P1,i)*probabilityB;
			else probabilityC+=(factorial(nNeighbor)/(factorial(i)*factorial(nNeighbor-i)))*pow(P1,nNeighbor-i)*pow(1-P1,i)*probabilityB;
			cout << endl;
		}
		probability=probabilityA*probabilityC;
		// cout << temp.level << "." << temp.position << "'s probability is " << probability << endl;
		return probability;
	}
}

struct sockaddr_in *GetAddrByNICName(string NICName)// 通过rtnetlink获取网口的地址
{
  struct ifaddrs *ifa;
  if (0!=getifaddrs(&ifa))
  {
    printf("getifaddrs error\n");
    return NULL;
  }
  for (;ifa!=NULL;)
  {
    if (ifa->ifa_name==NICName && ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL)
    {
      return (struct sockaddr_in *)(ifa->ifa_addr);
    }
    ifa=ifa->ifa_next;
  }
  return NULL;
}

int main(int argc, char const *argv[])
{
	printf("1\n");
	struct sockaddr_in *tempaddr=GetAddrByNICName(MGMT_INTERFACE);
	printf("2\n");
	if (tempaddr==NULL) printf("error\n");
	else printf("%s\n",inet_ntoa(tempaddr->sin_addr));
	// ident temp,nextIdent;
	// temp.level=atoi(argv[1]);
	// temp.position=atoi(argv[2]);
	// m_Node=temp;
	// // int interface,nextHopInterface=0;
	// // interface=atoi(argv[3]);
	// // m_nMaster=atoi(argv[3]);
	// // cout << pow(P2,m_nMaster) << endl;
	// cout << Probability(temp,-1) << endl;
	// // nextIdent=GetNextHopIdent(temp.level,temp.position,interface,&nextHopInterface);
	// // cout << nextIdent.level << "." << nextIdent.position << ",nextHopInterface is " << nextHopInterface << endl;
	return 0;
}