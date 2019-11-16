#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

// #define m_SpineNodes 16
// #define m_LeafNodes 4
// #define m_ToRNodes 1
// #define m_nPods 2
// #define m_AdvInterval 1

int m_AdvInterval,m_ToRNodes,m_LeafNodes,m_SpineNodes,m_nPods;

struct ident
{
	int level;
	int position;
};

string Allocate_ip(int level,int position,int interface)//position从1开始,interface从1开始
{
  stringstream ip_address;
  if (level==3)
  {
    ip_address << "32." << position << "." << interface << "." << 1;
  }
  else if (level==2)
  {
    if (interface<(m_SpineNodes/m_LeafNodes+1))
    {
      ip_address << "32." << (position-1)%m_LeafNodes*(m_SpineNodes/m_LeafNodes)+interface << "." << (position-1)/m_LeafNodes+1 << "." << 2;
    }
    else if (interface>=(m_SpineNodes/m_LeafNodes+1))
    {
      ip_address << "21." << position << "." << interface-(m_SpineNodes/m_LeafNodes) << "." << 1;
    }
  }
  else if (level==1)
  {
    ip_address << "21." << (position-1)/m_ToRNodes*m_LeafNodes+interface << "." << (position-1)%m_ToRNodes+1 << "." << 2;
  }
  return ip_address.str();
}

ident GetNextHopIdent(int level,int position,int interface,int * nextHopInterface)//interface从1开始,position从1开始
{
  ident nextHopIdent;
  if (level==1)//若是第一层的端口，则返回它的下一跳，即第二层的下行链路端口的位置信息
  {
    nextHopIdent.level=2;
    nextHopIdent.position=(position-1)/m_ToRNodes*m_LeafNodes+interface;
    *nextHopInterface=(position-1)%m_ToRNodes+(m_SpineNodes/m_LeafNodes)+1;
  }
  else if (level==2)
  {
    if (interface<((m_SpineNodes/m_LeafNodes)+1))//若是第二层的上行链路端口，则返回它的下一跳，即第三层的下行链路端口的位置信息
    {
      nextHopIdent.level=3;
      nextHopIdent.position=(position-1)%m_LeafNodes*(m_SpineNodes/m_LeafNodes)+interface;
      *nextHopInterface=(position-1)/m_LeafNodes+1;
    }
    else if (interface>=((m_SpineNodes/m_LeafNodes)+1))//若是第二层的下行链路端口，则返回它的下一跳，即第一层的上行链路端口的位置信息
    {
      nextHopIdent.level=1;
      nextHopIdent.position=(position-1)/m_LeafNodes*m_ToRNodes+interface-(m_SpineNodes/m_LeafNodes);
      *nextHopInterface=(position-1)%m_LeafNodes+1;
    }
  }
  else if (level==3)//若是第三层的下行链路端口，则返回它的下一跳，即第二层的上行链路端口的位置信息
  {
    nextHopIdent.level=2;
    nextHopIdent.position=(position-1)/(m_SpineNodes/m_LeafNodes)+1+(interface-1)*m_LeafNodes;
    *nextHopInterface=(position-1)%(m_SpineNodes/m_LeafNodes)+1;
  }
  return nextHopIdent;
}

int main(int argc, char const *argv[])
{
    m_AdvInterval=atoi(argv[1]);
    m_ToRNodes=atoi(argv[2]);
    m_LeafNodes=atoi(argv[3]);
    m_SpineNodes=atoi(argv[4]);
    m_nPods=atoi(argv[5]);
	struct ident m_ident;
	//获得地址后8位
	stringstream suffix;
	char buff[3];
	FILE * fpA;
	if (fpA=popen("ifconfig eth0 | grep \"inet addr:\" | awk '{print $2}' | cut -c 13-","r"))//截取eth0 ip的后两位
	{
		fgets(buff,sizeof(buff),fpA);
	}
	suffix << buff;
	int ipSuffix=atoi(suffix.str().c_str());
	//cout << "ipSuffix:" << ipSuffix << "\n";

    ofstream foutA("/usr/local/etc/bgpd.conf",ios::out);
    /*ofstream foutA("/home/guolab/bgpd.conf",ios::out);*/
    foutA << "! -*- bgp -*-" << "\n";
    foutA << "!" << "\n" << "! BGPd sample configuratin file" << "\n" << "!" << "\n" << "! $Id: bgpd.conf.sample,v 1.1 2002/12/13 20:15:29 paul Exp $" << "\n" << "!" << "\n";
    foutA << "hostname bgpd" << "\n" << "password zebra" << "\n";
    foutA << "debug bgp" << "\n";
    foutA << "debug bgp fsm" << "\n";
    foutA << "debug bgp events" << "\n";
    foutA << "debug bgp updates" << "\n";
    foutA << "!enable password please-set-at-here" << "\n" << "!" << "\n" << "!bgp mulitple-instance" << "\n" << "!" << "\n";
 
    //ASN and router_id
    if (ipSuffix<=75)//spine
    {
    	//cout << "spine" << "\n";
    	foutA << "router bgp 65001" << "\n";
    	foutA << " bgp router-id 30.0.0." << ipSuffix-59 << "\n";
    	m_ident.level=3;
    	m_ident.position=ipSuffix-59;

    	for (int i=1;i<=m_nPods;i++)//此处可改nPods
    	{
    		struct ident nextHopIdent;
    		int nextHopInterface=0;
    		string nextHopAddress;
    		nextHopIdent=GetNextHopIdent(m_ident.level,m_ident.position,i,&nextHopInterface);
    		nextHopAddress=Allocate_ip(nextHopIdent.level,nextHopIdent.position,nextHopInterface);
    		foutA << " neighbor " << nextHopAddress << " remote-as 65002" << "\n";
    		foutA << " neighbor " << nextHopAddress << " advertisement-interval " << m_AdvInterval << "\n";
    	}
    	foutA << " maximum-paths 64" << "\n";
    	foutA << "   redistribute connected" << "\n" << " address-family ipv4 unicast" << "\n";
    	for (int i=1;i<=m_nPods;i++)
    	{
    		struct ident nextHopIdent;
    		int nextHopInterface=0;
    		string nextHopAddress;
    		nextHopIdent=GetNextHopIdent(m_ident.level,m_ident.position,i,&nextHopInterface);
    		nextHopAddress=Allocate_ip(nextHopIdent.level,nextHopIdent.position,nextHopInterface);
    		foutA << "   neighbor " << nextHopAddress << " activate" << "\n";
    	}
    	foutA << "  exit-address-family" << "\n";
    }
    else if (ipSuffix>75 && ipSuffix<84)//leaf
    {
    	//cout << "leaf" << "\n";
        foutA << "router bgp 65002" << "\n";
        foutA << " bgp router-id 20.0.0." << ipSuffix-75 << "\n";
        m_ident.level=2;
    	m_ident.position=ipSuffix-75;

    	for (int i=1;i<=(m_SpineNodes/m_LeafNodes);i++)//与spine的邻居关系
    	{
    		struct ident nextHopIdent;
    		int nextHopInterface=0;
    		string nextHopAddress;
    		nextHopIdent=GetNextHopIdent(m_ident.level,m_ident.position,i,&nextHopInterface);
    		nextHopAddress=Allocate_ip(nextHopIdent.level,nextHopIdent.position,nextHopInterface);
    		foutA << " neighbor " << nextHopAddress << " remote-as 65001" << "\n";
    		foutA << " neighbor " << nextHopAddress << " advertisement-interval " << m_AdvInterval << "\n";
    	}
    	for (int i=(m_SpineNodes/m_LeafNodes)+1;i<=(m_SpineNodes/m_LeafNodes)+m_ToRNodes;i++)//此处可改nPods
    	{
    		struct ident nextHopIdent;
    		int nextHopInterface=0;
    		string nextHopAddress;
    		nextHopIdent=GetNextHopIdent(m_ident.level,m_ident.position,i,&nextHopInterface);
    		nextHopAddress=Allocate_ip(nextHopIdent.level,nextHopIdent.position,nextHopInterface);
    		foutA << " neighbor " << nextHopAddress << " remote-as 6500" << nextHopIdent.position+2 << "\n";
    		foutA << " neighbor " << nextHopAddress << " advertisement-interval " << m_AdvInterval << "\n";
    	}
    	for (int i=1;i<=(m_SpineNodes/m_LeafNodes);i++)//此处可改nPods
    	{
    		struct ident nextHopIdent;
    		int nextHopInterface=0;
    		string nextHopAddress;
    		nextHopIdent=GetNextHopIdent(m_ident.level,m_ident.position,i,&nextHopInterface);
    		nextHopAddress=Allocate_ip(nextHopIdent.level,nextHopIdent.position,nextHopInterface);
    		foutA << " neighbor " << nextHopAddress << " allowas-in 1" << "\n";
    	}
    	foutA << " maximum-paths 64" << "\n";
    	foutA << "   redistribute connected" << "\n" << " address-family ipv4 unicast" << "\n";
    	for (int i=1;i<=(m_SpineNodes/m_LeafNodes)+m_ToRNodes;i++)
    	{
    		struct ident nextHopIdent;
    		int nextHopInterface=0;
    		string nextHopAddress;
    		nextHopIdent=GetNextHopIdent(m_ident.level,m_ident.position,i,&nextHopInterface);
    		nextHopAddress=Allocate_ip(nextHopIdent.level,nextHopIdent.position,nextHopInterface);
    		foutA << "   neighbor " << nextHopAddress << " activate" << "\n";
    	}
    	foutA << "  exit-address-family" << "\n";
    } 
    else if (ipSuffix>=84)//tor
    {
    	//cout << "tor" << "\n";
    	// foutA << "router bgp " << 65002+ipSuffix-83 << "\n";
    	// foutA << " bgp router-id 10.0.0." << ipSuffix-83 << "\n";
    	// m_ident.level=1;
    	// m_ident.position=ipSuffix-83;
        if (ipSuffix==84) 
        {
           foutA << "router bgp " << 65003 << "\n"; 
           foutA << " bgp router-id 10.0.0." << 1 << "\n";
           m_ident.level=1;
           m_ident.position=1;
        }
        else if (ipSuffix==85) 
        {
           foutA << "router bgp " << 65005 << "\n"; 
           foutA << " bgp router-id 10.0.0." << 3 << "\n";
           m_ident.level=1;
           m_ident.position=3;
        }
        else if (ipSuffix==96) 
        {
           foutA << "router bgp " << 65004 << "\n"; 
           foutA << " bgp router-id 10.0.0." << 2 << "\n";
           m_ident.level=1;
           m_ident.position=2;
        }
        else if (ipSuffix==97) 
        {
           foutA << "router bgp " << 65006 << "\n"; 
           foutA << " bgp router-id 10.0.0." << 4 << "\n";
           m_ident.level=1;
           m_ident.position=4;
        }

    	for (int i=1;i<=m_LeafNodes;i++)//此处可改nPods
    	{
    		struct ident nextHopIdent;
    		int nextHopInterface=0;
    		string nextHopAddress;
    		nextHopIdent=GetNextHopIdent(m_ident.level,m_ident.position,i,&nextHopInterface);
    		nextHopAddress=Allocate_ip(nextHopIdent.level,nextHopIdent.position,nextHopInterface);
    		foutA << " neighbor " << nextHopAddress << " remote-as 65002" << "\n";
    		foutA << " neighbor " << nextHopAddress << " advertisement-interval " << m_AdvInterval << "\n";
    	}
    	foutA << " maximum-paths 64" << "\n";
    	foutA << "   redistribute connected" << "\n" << " address-family ipv4 unicast" << "\n";
    	for (int i=1;i<=m_LeafNodes;i++)
    	{
    		struct ident nextHopIdent;
    		int nextHopInterface=0;
    		string nextHopAddress;
    		nextHopIdent=GetNextHopIdent(m_ident.level,m_ident.position,i,&nextHopInterface);
    		nextHopAddress=Allocate_ip(nextHopIdent.level,nextHopIdent.position,nextHopInterface);
    		foutA << "   neighbor " << nextHopAddress << " activate" << "\n";
    	}
    	foutA << "  exit-address-family" << "\n";
    }
    foutA << "!" << "\n" << "! access-list all permit any" << "\n" << "!" << "\n" << "!route-map set-nexthop permit 10" << "\n" << "! match ip address all" << "\n";
    foutA << "! set ip next-hop 10.0.0.1" << "\n" << "!" << "\n" << "log file /home/guolab/output/bgpd.log" << "\n" << "!" << "\n" << "log stdout" << "\n";//log syslog 将日志输出到系统日志 log stdout 将日志输出到标准输出
    foutA.flush();
    foutA.close();

	return 0;
}