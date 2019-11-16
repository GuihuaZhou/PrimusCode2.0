#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <net/if.h>

using namespace std;

int main(int argc, char const *argv[])
{
	struct ifaddrs *ifa;
	while (1)//循环检测网络接口状态
	{
		//返回0表示成功，获取本地网络接口的信息，getifaddrs创建一个链表，链表上的每个节点都是一个struct ifaddrs结构
		if (0!=getifaddrs(&ifa))
		{
			printf("getifaddrs error\n");
			break;
		}

		for (;ifa!=NULL;)
		{
			if (ifa->ifa_flags==IFF_UP)
			{
				printf("hahah\n");
			}
			// if (ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL)
			if (ifa->ifa_flags==69699 && ifa->ifa_name!=NULL && ifa->ifa_addr!=NULL && ifa->ifa_netmask!=NULL && (*ifa).ifa_ifu.ifu_dstaddr!=NULL && ifa->ifa_name && (ifa->ifa_name[0]=='E' || ifa->ifa_name[0]=='e'))
			{
				string name=ifa->ifa_name;
				// printf("%s\t",ifa->ifa_name);
				cout << name << "\n";
				printf("%u\n",ifa->ifa_flags);
				printf("%d\n",((struct sockaddr_in*)(ifa->ifa_netmask))->sin_port);
				printf("%s\n",inet_ntoa(((struct sockaddr_in*)(ifa->ifa_addr))->sin_addr));
				printf("%s\n",inet_ntoa(((struct sockaddr_in*)(ifa->ifa_netmask))->sin_addr));
				printf("%d\n",((struct sockaddr_in*)(ifa->ifa_netmask))->sin_addr.s_addr);
				unsigned int mask=((struct sockaddr_in*)(ifa->ifa_netmask))->sin_addr.s_addr;
				int k=2;
				int counter=0;
				while (mask & 0x80000000)
				{
					// printf("yushu %ld\n",mask%k);
					// if (mask%k==1) counter++;
					// else if (mask%k==0) break;
					// printf("mask is%d\n",mask);
					// mask=mask/k;
					// counter++;
					// k*=2;
					mask << 1;
					counter++;
				}
				printf("%d\n",counter);
				printf("%s\n\n",inet_ntoa(((struct sockaddr_in*)((*ifa).ifa_ifu.ifu_dstaddr))->sin_addr));
			}
			ifa=ifa->ifa_next;
		}
		printf("\n");
		sleep(10);
		freeifaddrs(ifa);
		// break;
	}
	return 0;
}

// #include <stdio.h>
// #include <strings.h>

// #include <errno.h>
// #include <string.h>

// #include <asm/types.h>
// #include <linux/netlink.h>
// #include <linux/rtnetlink.h>
// #include <sys/socket.h>

// #include <sys/types.h>
// #include <sys/socket.h>
// #include <linux/if.h>
// #include <iostream>
// #include <unistd.h>
// #include <sys/ioctl.h>
// #include "/usr/include/net/if_arp.h"

// using namespace std;

// int main() {

//     int fd;
//     int ret;
//     struct sockaddr_nl addr;
//     char buf[20480] = {0};
//     int len = 1048576;

//     struct {
//         struct nlmsghdr nh;
//         struct ifinfomsg msg;
//     } req;

//     struct iovec iov;
//     struct msghdr msg;

//     fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
//     if (fd == -1)
//     {
//         printf("socket failed, %s\n", strerror(errno));
//         return -1;
//     }

//     ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
//     if (ret == -1) 
//     {
//         printf("setsockopt failed, %s\n", strerror(errno));
//         return -1;
//     }

//     bzero(&addr, sizeof(addr));
//     addr.nl_family = AF_NETLINK;
//     ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
//     if (ret == -1) 
//     {
//         printf("bind failed, %s\n", strerror(errno));
//         return -1;
//     }

//     bzero(&ret, sizeof(ret));
//     req.nh.nlmsg_len = NLMSG_ALIGN(sizeof(req));
//     req.nh.nlmsg_type = RTM_GETLINK;
//     req.nh.nlmsg_flags = NLM_F_ROOT | NLM_F_REQUEST;
//     req.nh.nlmsg_pid = 0;
//     req.nh.nlmsg_seq = 0;
//     req.msg.ifi_family = AF_UNSPEC;

//     while(1)
//     {
//     	ret = send(fd, &req, sizeof(req), 0);
// 	    if (ret == -1) 
// 	    {
// 	        printf("send failed, %s\n", strerror(errno));
// 	        return -1;
// 	    }

// 	    iov.iov_base = buf;
// 	    iov.iov_len = sizeof(buf);
// 	    msg.msg_name = &addr;
// 	    msg.msg_namelen = sizeof(addr);
// 	    msg.msg_iov = &iov;
// 	    msg.msg_iovlen = 1;

// 	    while(1) 
// 	    {
// 	        int found = 0;
// 	        struct nlmsghdr *nh;
// 	        ret = recvmsg(fd, &msg, 0);
// 	        if (ret == -1) 
// 	        {
// 	            if (errno == EINTR || errno == EAGAIN) continue;
// 	            printf("recvmsg failed, %s\n", strerror(errno));
// 	            return -1;
// 	        }

// 	        if (ret == 0) 
// 	        {
// 	            printf("recv EOF\n");
// 	            return -1;
// 	        }

// 	        nh = (struct nlmsghdr *)buf;

// 	        while(NLMSG_OK(nh, ret)) 
// 	        {
// 	            if (nh->nlmsg_type == NLMSG_DONE) 
// 	            {
// 	                found = 1;
// 	                break;
// 	            } 
// 	            else if (nh->nlmsg_type == NLMSG_ERROR)
// 	            {
// 	                printf("NLMSG_ERROR\n");
// 	                return -1;
// 	            }

// 	            struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA(nh);
// 	            struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(nh);
// 	            struct rtattr *rta = IFLA_RTA(ifi);
// 	            int payload_len = IFLA_PAYLOAD(nh);           

// 	            while(RTA_OK(rta, payload_len)) 
// 	            {
// 	            	if (rta->rta_type == IFLA_IFNAME) 
// 	            	{
// 						if (ifi->ifi_flags & IFF_RUNNING) 
// 						{ //判断网卡是否工作
// 							if (ifi->ifi_type==ARPHRD_ETHER)
// 							{//判断是否为Ethernet 10/100Mbps,ifi_type定义在 "/usr/include/net/if_arp.h"
// 								char *ifname = (char *)RTA_DATA(rta);
// 								printf("%d\t",ifa->ifa_family);
// 								printf("%d\t",ifa->ifa_prefixlen);
// 								printf("%d\t",ifa->ifa_flags);
// 								printf("%d\t",ifa->ifa_scope);
// 								printf("%d\n",ifa->ifa_index);
// 	                    		// printf("%s,index=%u,flag=%u,type=%u\n",ifname,ifi->ifi_index,ifi->ifi_flags,ifi->ifi_type);
// 	                    		// 此处为每个新的网卡建立线程，发送hello给邻居
// 	                    		// function future
// 							}
// 	                	}
// 	                }
// 	                rta = RTA_NEXT(rta, payload_len);
// 	            }
// 	            nh = NLMSG_NEXT(nh, ret);
// 	        }
// 	        if (found) 
// 	        {
// 	            break;
// 	        }
// 	    }
// 	    sleep(10);
// 	    printf("-----------------------\n\n");
//     }
//     return 0;
// }