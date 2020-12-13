#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include "primus.h"

#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>

#include "deps/raft/raft.h"

using namespace std;

int level=2;
int position=1;

int nPods=2;//Pod总数
int SpineNodes=16;//顶层交换机数
int LeafNodes=4;//单个pod里2层交换机数
int ToRNodes=1;//每个pod里的ToR交换机数
int defaultLinkTimer=1000;//ms
int defaultKeepaliveTimer=6;
vector<string> masterAddress;
int print_master_recv_all_LRs_time=false;//
int print_node_modify_time=false;
int print_node_recv_RS_time=false;
string mgmt_interface="eth0";

int
OpenNetlink(){
    
    struct sockaddr_nl saddr;

    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    if (sock < 0) {
        perror("Failed to open netlink socket");
        return -1;
    }

    memset(&saddr, 0, sizeof(saddr));

    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = 0;       //与内核通信,nl_pid设为0
    saddr.nl_pad = 0 ;
    saddr.nl_groups = 0;

    if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("Failed to bind to netlink socket");
        close(sock);
        return -1;
    }

    return sock;
}


int
addattr_l(struct nlmsghdr *n, size_t maxlen, int type, void *data, size_t alen)
{
  size_t len;
  struct rtattr *rta;

  len = RTA_LENGTH (alen);

  if (NLMSG_ALIGN (n->nlmsg_len) + len > maxlen)
    return -1;

  rta = (struct rtattr *) (((char *) n) + NLMSG_ALIGN (n->nlmsg_len));
  rta->rta_type = type;
  rta->rta_len = len;
  memcpy (RTA_DATA (rta), data, alen);
  n->nlmsg_len = NLMSG_ALIGN (n->nlmsg_len) + len;

  return 0;
}

void 
DelRoute(struct sockaddr_in destAddr,unsigned int prefixLen)
{

  struct {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[4096];
  } req;
  
  int rt_sock=OpenNetlink();
  memset(&req,0,sizeof(req));
  req.n.nlmsg_len=NLMSG_LENGTH(sizeof(struct rtmsg));
  req.n.nlmsg_flags=NLM_F_REQUEST | 0;
  req.n.nlmsg_type=RTM_DELROUTE;
  req.r.rtm_family=AF_INET;
  req.r.rtm_table=RT_TABLE_MAIN;
  req.r.rtm_protocol=RTPROT_ZEBRA;
  req.r.rtm_scope=RT_SCOPE_UNIVERSE;
  req.r.rtm_type=RTN_UNICAST;
  req.r.rtm_dst_len=prefixLen;

  //destAddr.sin_addr.s_addr=destAddr;
  addattr_l(&req.n,sizeof(req),RTA_DST,&(destAddr.sin_addr.s_addr),4);
  
  int status=send(rt_sock,&req,req.n.nlmsg_len,0);
  close(rt_sock);

}


int send_get_route_requst(int sock)
{
    struct {
        struct nlmsghdr n;
        struct rtmsg r;
    } nl_req;

    nl_req.n.nlmsg_type = RTM_GETROUTE;
    nl_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nl_req.n.nlmsg_len = sizeof(nl_req);
    nl_req.n.nlmsg_seq = time(NULL);
    nl_req.r.rtm_family = AF_INET;
    nl_req.r.rtm_table = RT_TABLE_MAIN;    //doesn't work, i don't know
    nl_req.r.rtm_protocol = RTPROT_ZEBRA;   //doesn't work, i don't know. oh, fuck !!!

    return send(sock, &nl_req, sizeof(nl_req), 0);
}

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
    memset(tb, 0, sizeof(struct rtattr *) * (max + 1));

    while (RTA_OK(rta, len)) {
        if (rta->rta_type <= max) {
            tb[rta->rta_type] = rta;
        }

        rta = RTA_NEXT(rta,len);
    }
}

static inline int rtm_get_table(struct rtmsg *r, struct rtattr **tb)
{
    __u32 table = r->rtm_table;

    if (tb[RTA_TABLE]) {
        table = *(__u32 *)RTA_DATA(tb[RTA_TABLE]);
    }

    return table;
}

static inline int rtm_get_protocol_num(struct rtmsg *r, struct rtattr **tb)
{
    __u32 protocol_num = r->rtm_protocol;

    return protocol_num;
}

struct DestNetmaskProtoNum{
    
    int protocolNum;
    int netmask;
    __u32 des;
};

void
store_DestNetmaskProtoNum_info(struct nlmsghdr* nl_header_answer, vector<struct DestNetmaskProtoNum> * destNetmaskProtoNum_vector){
    
    struct rtmsg* r =(rtmsg*) NLMSG_DATA(nl_header_answer);
    int len = nl_header_answer->nlmsg_len;
    struct rtattr* tb[RTA_MAX+1];
    int route_table;
    int protocol_num;
    struct DestNetmaskProtoNum destNetmaskProtoNum;

    len -= NLMSG_LENGTH(sizeof(*r));     //去除rtmsg和nlmsg后的长度

    if (len < 0) {
        perror("Wrong message length");
        return;
    }
    
    parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);

    route_table = rtm_get_table(r, tb);
    if(route_table!=RT_TABLE_MAIN){
        return;
    }
    protocol_num=rtm_get_protocol_num(r, tb);

    destNetmaskProtoNum.protocolNum=protocol_num;

    if (tb[RTA_DST]) {
        destNetmaskProtoNum.netmask=r->rtm_dst_len;
        //cout<<"输出输出输出呢；:::::"<<destNetmaskProtoNum.netmask<<endl;
        destNetmaskProtoNum.des=*(__u32*)RTA_DATA(tb[RTA_DST]);
        (*destNetmaskProtoNum_vector).push_back(destNetmaskProtoNum);
        }
}

void 
get_route_dump_response(int sock, vector<struct DestNetmaskProtoNum> *destNetmaskProtoNum_vector)
{
    struct sockaddr_nl nladdr;
    struct iovec iov;
    struct msghdr msg = {
        .msg_name = &nladdr,
        .msg_namelen = sizeof(nladdr),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    char* recv_buf=(char*)malloc(4096*2);                   //缓冲区大小,上万条路由缓冲区应设为多大???
    iov.iov_base=recv_buf;
    iov.iov_len=4096*2;
    int recv_len = recvmsg(sock, &msg, 0);
    
    struct nlmsghdr *h = (struct nlmsghdr *)recv_buf;
    int msglen = recv_len;

    printf("Main routing table IPv4\n");
    if (h->nlmsg_type == NLMSG_DONE)
            return ;
    while (NLMSG_OK(h, msglen)) 
    {
        if (h->nlmsg_flags & NLM_F_DUMP_INTR) {
            fprintf(stderr, "Dump was interrupted\n");
            free(recv_buf);
            //return ;
        }

        if (nladdr.nl_pid != 0) {
            continue;
        }

        if (h->nlmsg_type == NLMSG_ERROR) {
            perror("netlink reported error");
            free(recv_buf);
        }

        store_DestNetmaskProtoNum_info(h, destNetmaskProtoNum_vector);

        h = NLMSG_NEXT(h, msglen);
    }
    free(recv_buf);


}

static void* get_destNetmaskProtoNum_info_all(void* destNetmaskProtoNum_vector){
    
    int sock=OpenNetlink();
    send_get_route_requst(sock);
    get_route_dump_response(sock, (vector<struct DestNetmaskProtoNum>*)destNetmaskProtoNum_vector);
    return ((void *)0);
}

pid_t getDaemon(pid_t primusPid, uint32_t intervalSeconds)
{
  /***
  功能：primus守护进程：监听primus进程是否挂掉，primus挂了后自动退出
  参数：
        primusPid: primus pid,由getpid()获得
      intervalSeconds: 监听时间间隔(s)

  ***/
  pid_t ppid;
  pid_t pid;
  ppid = fork();

  if(ppid == 0)
  {
    pid = daemon(0,0);
    if(ppid == 0 && pid != 0)
    {
      exit(0);
    }
    if(pid == -1)
    {
      perror( "error" );
    }
    else
    {             
      prctl(PR_SET_NAME, "primusDaemon");    //cat cmd: cat /proc/pidNum/status
      while(1)
      {
        if( kill( primusPid, 0 ) == -1 )
        {
          // made by boss lin
          struct sockaddr_in sockaddr;
          vector<struct DestNetmaskProtoNum> destNetmaskProtoNum_vector;//把每一条路由的协议号、网络号和掩码存储在destNetmaskProtoNum_vector向量里
          vector<struct DestNetmaskProtoNum> * p_destNetmaskProtoNum_vector=&destNetmaskProtoNum_vector;
            
          get_destNetmaskProtoNum_info_all(p_destNetmaskProtoNum_vector);
          for(int i=0;i<(*p_destNetmaskProtoNum_vector).size(); i++)
          {
              if ((*p_destNetmaskProtoNum_vector)[i].protocolNum == RTPROT_ZEBRA)
              {
                  sockaddr.sin_family = AF_INET;
                  sockaddr.sin_addr.s_addr = (*p_destNetmaskProtoNum_vector)[i].des;
                  DelRoute( sockaddr, (*p_destNetmaskProtoNum_vector)[i].netmask);    //网络字节序IP
              }
          }
          // end
          // m_globalRouting->~Primus();
          exit(0);
        }
        else
        {
          sleep(intervalSeconds);
        }
      }
    }     
  }
  return ppid; 
}

int main(int argc,char *argv[])
{
  ifstream fin("/usr/local/etc/Primus.conf",ios::in);
  
  string config;
  int begin,end;

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  masterAddress.push_back(config.substr(begin,config.length()-begin));

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  for (int i=begin;i<config.length();)
  {
    while (i<config.length() && config[i]!=',') i++;
    masterAddress.push_back(config.substr(begin,i-begin));
    begin=++i;
  }

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  level=atoi(config.substr(begin,config.length()-begin).c_str());

  config="";
  getline(fin,config);
  begin=config.find(':',0)+1;
  position=atoi(config.substr(begin,config.length()-begin).c_str());

  // config="";
  // getline(fin,config);
  // begin=config.find(':',0)+1;
  // defaultLinkTimer=atoi(config.substr(begin,config.length()-begin).c_str());

  // config="";
  // getline(fin,config);
  // begin=config.find(':',0)+1;
  // defaultKeepaliveTimer=atoi(config.substr(begin,config.length()-begin).c_str());

  // config="";
  // getline(fin,config);
  // begin=config.find(':',0)+1;
  // ToRNodes=atoi(config.substr(begin,config.length()-begin).c_str());

  // config="";
  // getline(fin,config);
  // begin=config.find(':',0)+1;
  // LeafNodes=atoi(config.substr(begin,config.length()-begin).c_str());

  // config="";
  // getline(fin,config);
  // begin=config.find(':',0)+1;
  // SpineNodes=atoi(config.substr(begin,config.length()-begin).c_str());

  // config="";
  // getline(fin,config);
  // begin=config.find(':',0)+1;
  // nPods=atoi(config.substr(begin,config.length()-begin).c_str());

  fin.close();

  ToRNodes=atoi(argv[1]);
  LeafNodes=atoi(argv[2]);
  SpineNodes=atoi(argv[3]);
  nPods=atoi(argv[4]);
  print_master_recv_all_LRs_time=atoi(argv[5]);//
  print_node_modify_time=atoi(argv[6]);
  print_node_recv_RS_time=atoi(argv[7]);
  mgmt_interface=argv[8];

  // raft_server_t* raft = raft_new();

  Primus *m_Primus=new Primus(level,
    position,
    ToRNodes,
    LeafNodes,
    SpineNodes,
    nPods,
    defaultLinkTimer,
    defaultKeepaliveTimer,
    masterAddress,
    print_master_recv_all_LRs_time,
    print_node_modify_time,
    print_node_recv_RS_time,
    mgmt_interface);

  pid_t pid=waitpid(getDaemon(getpid(),3),NULL,0);
  m_Primus->Start();
  pthread_exit(NULL);
  return 0;
}