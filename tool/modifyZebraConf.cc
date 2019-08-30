#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

int main(int argc, char const *argv[])
{
	ofstream foutA("/usr/local/etc/zebra.conf",ios::out);
	foutA << "! -*- zebra -*-\n!\n! zebra sample configuration file\n!\n! $Id: zebra.conf.sample,v 1.1 2002/12/13 20:15:30 paul Exp $\n!\n";
	foutA << "hostname Router\npassword zebra\nenable password zebra\n";
	//foutA << "debug zebra events\n";
	//foutA << "debug zebra packet\n";
	//foutA << "debug zebra rib\n";
	foutA << "!\n! Interface's description.\n!\n!interface lo\n! description test of desc.\n!\n";
	foutA << "!interface sit0\n! multicast\n\n!\n! Static default route sample.\n!\n!ip route 0.0.0.0/0 203.181.89.241\n!\n";
	foutA << "\n!log file /home/guolab/output/zebra.log\n";
	return 0;
}