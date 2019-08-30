#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

int main(int argc, char const *argv[])
{
	string strA,strB;
	int counter=0;
	double totalTime=0,timeA=0,timeB=0;
	ifstream finA("/home/guolab/output/transmissionTimeA.txt",ios::app);
	ifstream finB("/home/guolab/output/transmissionTimeB.txt",ios::app);
	ofstream foutA("/home/guolab/output/transmissionTotalTime.txt",ios::app);
	getline(finA,strA);
	getline(finA,strA);
	getline(finB,strB);
	getline(finB,strB);
	foutA << strA.substr(9,4) << '\t';
	while (1)
	{
		if (!getline(finA,strA)) break;
		if (!getline(finB,strB)) break;
		if (strA.length()==0 || strB.length()==0) 
		{
			foutA << totalTime/counter << endl;
			totalTime=0;
			counter=0;
			continue;
		}
		if (strA[0]=='T' || strB[0]=='T') 
		{
			foutA << strA.substr(9,4) << '\t';
			continue;
		}
		timeA=atof(strA.c_str());
		timeB=atof(strB.c_str());
		if (timeA>timeB) totalTime+=timeA;
		else totalTime+=timeB;
		counter++;
	}
	foutA << totalTime/counter << endl;
	return 0;
}