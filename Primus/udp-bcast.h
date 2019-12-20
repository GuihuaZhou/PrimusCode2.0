
/*#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <linux/in.h>
#include <stdlib.h>*/

#include <string.h>
#include <sys/socket.h>
#include <iostream>

#define IP_FOUND "IP_FOUND"
#define IP_FOUND_ACK "IP_FOUND_ACK"
#define PORT 2224
#define MCAST_PORT 2224


#define TTL 1
#define BUF_SIZE 256

using namespace std;

class Node;
class UDPServerBcast{
public:
	//Set base information
	UDPServerBcast();
	~UDPServerBcast();
    bool StartApplication();
    void SetNode(Node *node);

private:
	static void* HandleRead(void* object);

	static Node* m_node;
};



class UDPClientBcast{
public:
	//Set base information
	UDPClientBcast();
	~UDPClientBcast();
    bool StartApplication();
    void SayHelloToAdjacency();
    void SetNode(Node *node);

private:
	static void* SayHelloThread(void* object);
	static void* HandleRead(void* object);

	static Node* m_node;
	
};