#include <iostream>
#include "ipv4-global-routing.h"

typedef void (*signalHandler)(int);

#define MAX_ROUTE 1000
using namespace std;

vector<string> MasterAddress;
int masterPort;
int level=2;
int position=1;

int nPods=2;//Pod总数
int addPods=0;
int SpineNodes=16;//顶层交换机数
int LeafNodes=4;//单个pod里2层交换机数
int ToRNodes=1;//每个pod里的ToR交换机数
int AllNodes=26;//节点总数
int Links=832;//链路总数
int nMaster=1;
int pod=0;
int defaultLinkTimer=100000;//us
int defaultKeepaliveTimer=6;

Ipv4GlobalRouting *m_globalRouting;

void SignalHandler(int sigNum)
{
  // 
}

int main(int argc,char *argv[])
{
  // ofstream Logfout("/var/log/Primus.log",ios::trunc);
  ifstream fin("/usr/local/etc/Primus.conf",ios::in);
  string config;
  int begin,end;

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  // masterAddress=config.substr(begin,end);
  MasterAddress.push_back(config.substr(begin,end));

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  masterPort=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  // level=atoi(config.substr(begin,end).c_str());
  level=atoi(argv[1]);

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  // position=atoi(config.substr(begin,end).c_str());
  position=atoi(argv[2]);

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  // defaultLinkTimer=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  defaultKeepaliveTimer=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  ToRNodes=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  LeafNodes=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  SpineNodes=atoi(config.substr(begin,end).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  end=config.length()-begin;
  nPods=atoi(config.substr(begin,end).c_str());

  // test
  MasterAddress.push_back("10.0.1.68");
  MasterAddress.push_back("10.0.1.69");
  // Logfout << "level:" << level << endl << "position:" << position << endl << "masterAddress:" << masterAddress << endl << "masterPort:" << masterPort << endl << "defaultLinkTimer:" << defaultLinkTimer << endl << "defaultKeepaliveTimer:" << defaultKeepaliveTimer << endl << endl;
  // end

  int Links=(SpineNodes+LeafNodes*ToRNodes)*nPods;
  if (level==3 || level==0) pod=0;
  else if (level==2) pod=position/LeafNodes+1;
  else if (level==1) pod=position/ToRNodes+1;
  // signal(SIGHUP,SignalHandler);
  // signal(SIGUSR1,SignalHandler);
  // signal(SIGINT,SignalHandler);
  // signal(SIGTERM,SignalHandler);
  m_globalRouting=new Ipv4GlobalRouting(level,position,ToRNodes,LeafNodes,SpineNodes,nPods,pod,nMaster,Links,defaultLinkTimer,defaultKeepaliveTimer,true,true);
  m_globalRouting->Start(MasterAddress);
  
  return 0;
}