#include <stdio.h>
#include <net/if.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <sys/types.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <string>
using namespace std;


struct NexthopAndWeight    //多路径路由使用
  {
    string NICName;        //本地接口,如"eth0"
    int weight;            //路由权重
  };


int
rta_addattr_l (struct rtattr *rta, size_t maxlen, int type, void *data, 
               size_t alen)
{
  size_t len;
  struct rtattr *subrta;

  len = RTA_LENGTH (alen);

  if (RTA_ALIGN (rta->rta_len) + len > maxlen)
    return -1;

  subrta = (struct rtattr *) (((char *) rta) + RTA_ALIGN (rta->rta_len));
  subrta->rta_type = type;
  subrta->rta_len = len;
  memcpy (RTA_DATA (subrta), data, alen);
  rta->rta_len = NLMSG_ALIGN (rta->rta_len) + len;

  return 0;
}

int
addattr_l (struct nlmsghdr *n, size_t maxlen, int type, void *data, size_t alen)
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

int
open_netlink(){
	
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

void
route_add_single_path(struct sockaddr_in sock_addr_in, string NICName , unsigned int netmask){    //单路径路由使用

	/***
	功能：向内核路由表添加单路径路由
	参数：
   sock_addr_in: 目的网络, struct sockaddr_in结构
	    NICName：接口名称, 如"eth0"
		netmask：掩码长度

	***/
	
    struct {
        struct nlmsghdr n;
        struct rtmsg r;
        char nl_buf[4096];
    } nl_req;
	
	int if_index=if_nametoindex(NICName.c_str());    //接口名称转索引
	int rt_sock=open_netlink();
	memset(&nl_req, 0, sizeof(nl_req));
    nl_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));    //NLMSG_LENGTH宏：返回nlmsghdr长度加上sizeof(struct rtmsg)长度
    nl_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_CREATE;           //没有则创建,有则替换,这相当于包含了修改功能
	nl_req.n.nlmsg_type=RTM_NEWROUTE;
	nl_req.r.rtm_family = AF_INET;
	nl_req.r.rtm_table = RT_TABLE_MAIN;
	nl_req.r.rtm_protocol = RTPROT_BOOT;
	nl_req.r.rtm_scope = RT_SCOPE_UNIVERSE;
	nl_req.r.rtm_type = RTN_UNICAST;
	nl_req.r.rtm_dst_len=netmask;
	nl_req.r.rtm_src_len=0;
	nl_req.r.rtm_tos=0;
	//nl_req.r.rtm_flags=RT_TABLE_MAIN;

    addattr_l (&nl_req.n, sizeof(nl_req), RTA_DST, &(sock_addr_in.sin_addr.s_addr), 4);
	addattr_l(&nl_req.n, sizeof(nl_req), RTA_OIF, &if_index, 4);
    
    
    int status=send(rt_sock,&nl_req,nl_req.n.nlmsg_len,0);
    fprintf(stderr,"status=%d,route add successfully\n",status);
	
}


void
route_add_multi_path(struct sockaddr_in sock_addr_in, unsigned int netmask, vector<struct NexthopAndWeight> nexthopAndWeight_vector)
{    //多路径路由使用
	
    struct {
        struct nlmsghdr n;
        struct rtmsg r;
        char nl_buf[2048];
    } nl_req;
	memset(&nl_req, 0, sizeof(nl_req));
	
	char rta_buf[2048];
	struct rtattr *rta =(struct rtattr *)((void *) rta_buf);
	struct rtnexthop *rtnh;
	
    nl_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nl_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_CREATE;
	nl_req.n.nlmsg_type=RTM_NEWROUTE;
	nl_req.r.rtm_family = AF_INET;
	nl_req.r.rtm_table = RT_TABLE_MAIN;
	nl_req.r.rtm_protocol = RTPROT_BOOT;
	nl_req.r.rtm_scope = RT_SCOPE_UNIVERSE;
	nl_req.r.rtm_type = RTN_UNICAST;
	nl_req.r.rtm_dst_len=netmask;     //netmask位数
	nl_req.r.rtm_src_len=0;
	nl_req.r.rtm_tos=0;
	//nl_req.r.rtm_flags=RT_TABLE_MAIN;
	
	//内核通信数据包attribute构建
	addattr_l (&nl_req.n, 2048, RTA_DST, &(sock_addr_in.sin_addr.s_addr), 4);
	
	rta->rta_type = RTA_MULTIPATH;
    rta->rta_len = RTA_LENGTH (0);
	rtnh = (struct rtnexthop *)RTA_DATA (rta);
	
	for (int i = 0; i<nexthopAndWeight_vector.size(); i++){
		
		rtnh->rtnh_len=sizeof(struct rtnexthop);
	    rtnh->rtnh_flags=0;//RTNH_F_ONLINK;
		rtnh->rtnh_hops=nexthopAndWeight_vector[i].weight-1;
		rtnh->rtnh_ifindex=if_nametoindex((nexthopAndWeight_vector[i].NICName).c_str());
		rta->rta_len = rta->rta_len+RTNH_LENGTH(0);
		rtnh=RTNH_NEXT(rtnh);
	}
	
	addattr_l (&nl_req.n, 4096, RTA_MULTIPATH, RTA_DATA (rta), RTA_PAYLOAD (rta));
	int rt_sock=open_netlink();
    int status=send(rt_sock,&nl_req,nl_req.n.nlmsg_len,0);
    fprintf(stderr,"status=%d,route add successfully\n",status);	
    
}

void route_del(struct sockaddr_in sock_addr_in, unsigned int netmask){
	
	/***
	功能：向内核路由表删除路由(e.g. route_del(sock_addr_in, 24))
	参数：
	     des_ip: 目的网络,32位网络字节序地址
		netmask: 掩码长度

	***/
	
    struct {
        struct nlmsghdr n;
        struct rtmsg r;
        char buf[4096];
    } nl_req;
	
	//__u32 des=des_ip;
	
	int rt_sock=open_netlink();
	memset(&nl_req, 0, sizeof(nl_req));
    nl_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nl_req.n.nlmsg_flags = NLM_F_REQUEST | 0;
	nl_req.n.nlmsg_type=RTM_DELROUTE;
	nl_req.r.rtm_family = AF_INET;
	nl_req.r.rtm_table = RT_TABLE_MAIN;
	nl_req.r.rtm_protocol = RTPROT_BOOT;
	nl_req.r.rtm_scope = RT_SCOPE_UNIVERSE;
	nl_req.r.rtm_type = RTN_UNICAST;
	nl_req.r.rtm_dst_len=netmask;
	nl_req.r.rtm_src_len=0;
	nl_req.r.rtm_tos=0;
	//nl_req.r.rtm_flags=RT_TABLE_MAIN;

	addattr_l(&nl_req.n, sizeof(nl_req), RTA_DST, &(sock_addr_in.sin_addr.s_addr), 4);

    
    int status=send(rt_sock,&nl_req,nl_req.n.nlmsg_len,0);
    fprintf(stderr,"status=%d,route delete successfully\n",status);
	
}

int
main(int argc, char ** argv){
	
    struct sockaddr_in mysock1, mysock2 ,mysock3;

	mysock1.sin_family = AF_INET;  
    mysock1.sin_port = htons(666);  
    mysock1.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	
	mysock2.sin_family = AF_INET; 
    mysock2.sin_port = htons(888); 
    mysock2.sin_addr.s_addr = inet_addr("21.2.1.1"); 
	
	mysock3.sin_family = AF_INET; 
    mysock3.sin_port = htons(0); 
    mysock3.sin_addr.s_addr = inet_addr("222.222.222.0"); 
	
	vector<struct NexthopAndWeight> NexthopAndWeight_vector;
	struct NexthopAndWeight nexthopAndWeight;
	nexthopAndWeight.weight=20;
	nexthopAndWeight.NICName="eth1";
	
	NexthopAndWeight_vector.push_back(nexthopAndWeight);
	nexthopAndWeight.weight=10;
	nexthopAndWeight.NICName="eth2";
	NexthopAndWeight_vector.push_back(nexthopAndWeight);
	route_add_multi_path(mysock3,24,NexthopAndWeight_vector);
	// route_add_single_path(mysock3, "eth0", 24);
	//route_del(mysock3.sin_addr.s_addr, 24);
	
	return 0;
}
