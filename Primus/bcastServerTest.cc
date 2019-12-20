#include<iostream>
#include <unistd.h>
#include "udp-bcast.h"
using namespace std;

int main()
{
	/*UDPClientBcast client;*/
	UDPServerBcast server;
	printf("begin\n");
	server.StartApplication();
	/*client.StartApplication();*/
	for(int i=0;i<100;i++)
	{
        sleep(1);
	}
	printf("end\n");
	return 1;

}