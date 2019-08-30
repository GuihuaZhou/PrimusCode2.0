#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

int main(int argc, char const *argv[])
{
	ofstream foutA("/home/guolab/output/transmissionTime.txt",ios::app);
	ofstream foutB("/home/guolab/output/throughputRatio.txt",ios::app);
	foutA << endl << "Timer is " << argv[1] << "ms" << endl;
	foutB << endl << "Timer is " << argv[1] << "ms" << endl;
	return 0;
}