#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

int main(int argc, char const *argv[])
{
  ifstream finA("/usr/local/etc/bgpd.conf",ios::app);
  string str;
  int advTime=atoi(argv[1]);
  while (getline(finA,str))
  {
    if (str[0]==' ' && str[1]=='n' && str[2]=='e')
    {
      int len=str.length();
      // str[len-1]=to_string(advTime);
      str[len-1]='0'+advTime;
    }
  }
	return 0;
}