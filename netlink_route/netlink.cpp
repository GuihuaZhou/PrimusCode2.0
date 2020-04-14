#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>

#define ERR_RET(x) do { perror(x); return EXIT_FAILURE; } while (0);
#define BUFFER_SIZE 4095

using namespace std;

string GetNow()
{
  time_t tt;
  time( &tt );
  tt = tt + 8*3600;  // transform the time zone
  tm* t= gmtime( &tt );
  stringstream time;
  time << "[" << t->tm_year+1900 << "-" << t->tm_mon+1 << "-" << t->tm_mday << " " << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]";
  return time.str();
}

int  loop (int sock, struct sockaddr_nl *addr)
{
    ofstream Logfout("/home/guolab/netlink_route/center.log",ios::app);
    Logfout << GetNow() << "hello 1----------------" << endl;
    int     received_bytes = 0;
    struct  nlmsghdr *nlh;
    char    destination_address[32];
    char    gateway_address[32];
    char    root_protocol ;
    struct  rtmsg *route_entry;  /* This struct represent a route entry in the routing table */
    struct  rtattr *route_attribute; /* This struct contain route attributes (route type) */
    int     route_attribute_len = 0;
    char    buffer[BUFFER_SIZE];
    
    bzero(destination_address, sizeof(destination_address));
    bzero(gateway_address, sizeof(gateway_address));
    bzero(buffer, sizeof(buffer));

    /* Receiving netlink socket data */
    while (1) {
        Logfout << GetNow() << "hello 2----------------" << endl;
        received_bytes = recv(sock, buffer, sizeof(buffer), 0);
        Logfout << GetNow() << "hello 2----------------" << endl;
        if (received_bytes < 0)
            ERR_RET("recv");
        /* cast the received buffer */
            nlh = (struct nlmsghdr *) buffer;
        /* If we received all data ---> break */
        if (nlh->nlmsg_type == NLMSG_DONE)
            break;
        /* We are just intrested in Routing information */
        if (addr->nl_groups == RTMGRP_IPV4_ROUTE)
            break;
        Logfout << GetNow() << "hello 3----------------" << endl;
        }

    // Logfout << GetNow() << "hello 2----------------" << endl;
/* Reading netlink socket data */
/* Loop through all entries */
/* For more informations on some functions :
 * http://www.kernel.org/doc/man-pages/online/pages/man3/netlink.3.html
 * http://www.kernel.org/doc/man-pages/online/pages/man7/rtnetlink.7.html
 */

    for ( ; NLMSG_OK(nlh, received_bytes); nlh = NLMSG_NEXT(nlh, received_bytes))
    {
        /* Get the route data */
        route_entry = (struct rtmsg *) NLMSG_DATA(nlh);//NLMSG_DATA(nlh): 该宏用于返回Netlink消息中数据部分的首地址，在写入和读取消息数据部分时会用到它。
        Logfout << GetNow() << "打印数据：" << std::endl;
        /* We are just intrested in main routing table */
        if (route_entry->rtm_table != RT_TABLE_MAIN)//RT_TABLE_MAIN主表
            continue;
        Logfout << GetNow() << "路由协议："<<route_entry->rtm_protocol<< std::endl;
        
        /* Get attributes of route_entry */
        route_attribute = (struct rtattr *) RTM_RTA(route_entry);

        /* Get the route atttibutes len */
        route_attribute_len = RTM_PAYLOAD(nlh);
        
        /* Loop through all attributes */
        for ( ; RTA_OK(route_attribute, route_attribute_len); route_attribute = RTA_NEXT(route_attribute, route_attribute_len))
        {
            /* Get the destination address */
            if (route_attribute->rta_type == RTA_DST)// RTA_DST协议地址路由目标地址。
            {
                inet_ntop(AF_INET, RTA_DATA(route_attribute), destination_address, sizeof(destination_address));
                Logfout << GetNow() << "路由目标地址："<<destination_address<< std::endl;
            }
            /* Get the gateway (Next hop) */
            if (route_attribute->rta_type == RTA_GATEWAY)//RTA_GATEWAY协议地址路由的网关
            {
                inet_ntop(AF_INET, RTA_DATA(route_attribute), gateway_address, sizeof(gateway_address));
                Logfout << GetNow() << "网关："<<gateway_address<< std::endl;
            }  
        }

        /* Now we can dump the routing attributes 
        if (nlh->nlmsg_type == RTM_DELROUTE)
            fprintf(stdout, "Deleting route to destination --> %s and gateway %s\n", destination_address, gateway_address);
        if (nlh->nlmsg_type == RTM_NEWROUTE)
            printf("Adding route to destination --> %s and gateway %s\n", destination_address, gateway_address);*/
        }
    return 0;
}

int main(int argc, char **argv)
{
    int sock = -1;
    struct sockaddr_nl addr;

    /* Zeroing addr */
    bzero (&addr, sizeof(addr));
    
    if ((sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0)
        ERR_RET("socket");

    addr.nl_family = AF_NETLINK;
    // addr.nl_pid = 0;
    addr.nl_groups = RTMGRP_IPV4_ROUTE;
    
    if (bind(sock,(struct sockaddr *)&addr,sizeof(addr)) < 0)
        ERR_RET("bind");
        
    while (1)
        loop (sock, &addr);
        
    /* Close socket */
    close(sock);
    return 0;
}