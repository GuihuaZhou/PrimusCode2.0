#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 4095

int  loop (int sock, struct sockaddr_nl *addr)
{
	ofstream Logfout("/home/guolab/netlink_route/center.log",ios::app);
    int     received_bytes = 0;
    struct  nlmsghdr *nlh;
    char    destination_address[32];
    char    gateway_address[32];
    struct  rtmsg *route_entry;  /* This struct represent a route entry in the routing table */
    struct  rtattr *route_attribute; /* This struct contain route attributes (route type) */
    int     route_attribute_len = 0;
    char    buffer[BUFFER_SIZE];
    
    bzero(destination_address, sizeof(destination_address));
    bzero(gateway_address, sizeof(gateway_address));
    bzero(buffer, sizeof(buffer));


    

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
    addr.nl_groups = RTMGRP_IPV4_ROUTE;
    
    if (bind(sock,(struct sockaddr *)&addr,sizeof(addr)) < 0)
        ERR_RET("bind");
        
    while (1)
        loop (sock, &addr);
        
    /* Close socket */
    close(sock);
    return 0;
}