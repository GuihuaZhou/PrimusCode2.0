#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <vector>
#define pi 3.1415926535

using namespace std;

FILE *fp;
int meanA=0;
int meanB=0;
int times=100;
int interval=100000;
bool start=false;
double failure_startTime_mu=0.25;
double failure_startTime_sigma=0;

struct tempthreadparam
{
	string IPA;
	string IPB;
	string ethA;
	string ethB;
	string type;
};

double UniformDistribution(double min, double max)
{
  return (double) (max-min)*((double) (rand()%RAND_MAX+1))/RAND_MAX+min;
}

double NormalDistribution(double mu,double sigma)
{
  double u1=UniformDistribution(0,1);
  double u2=UniformDistribution(0,1);
  double y=(double) sqrt(-2*log(u1))*sin(2*pi*u2);
  return mu+sigma*y;
}

double LogNormalDistribution(double mu,double sigma)
{
  double y=NormalDistribution(mu,sigma);
  return (double)exp(y);
}

void* ProcessThread(void* tempThreadParam)
{
	string IPA=((struct tempthreadparam *)tempThreadParam)->IPA;
	string IPB=((struct tempthreadparam *)tempThreadParam)->IPB;
	string ethA=((struct tempthreadparam *)tempThreadParam)->ethA;
	string ethB=((struct tempthreadparam *)tempThreadParam)->ethB;
	string type=((struct tempthreadparam *)tempThreadParam)->type;

	while (1)
	{
		if (start==true) break;
		usleep(100000);
	}
	string commandA,commandB,commandC,commandD;
	int waitTime=0;

	if (type=="BGP")// BGP
	{
		while (1)
		{
			commandA="ssh root@"+IPA+" ifconfig "+ethA+" down";
			commandB="ssh root@"+IPB+" ifconfig "+ethB+" down";
			commandC="ssh root@"+IPA+" ifconfig "+ethA+" up";
			commandD="ssh root@"+IPB+" ifconfig "+ethB+" up";
			waitTime=(int)(LogNormalDistribution(failure_startTime_mu,failure_startTime_sigma)*meanA);
			usleep(waitTime);
			// down
			system(commandA.c_str());
			system(commandB.c_str());
			usleep(interval);
			// up
			system(commandC.c_str());
			system(commandD.c_str());

			fseek(fp,0L,SEEK_END);
			fprintf(fp,(commandA+"\n").c_str());
			fprintf(fp,(commandB+"\n").c_str());
			fprintf(fp,(commandC+"\n").c_str());
			fprintf(fp,(commandD+"\n\n").c_str());
			fflush(fp);
		}
	}
	else if (type=="Primus")// Primus
	{
		while (1)
		{
			commandA="ssh root@"+IPA+" ifconfig "+ethA+" down";
			// commandB="ssh root@"+IPB+" ifconfig "+ethB+" down";
			commandC="ssh root@"+IPA+" ifconfig "+ethA+" up";
			// commandD="ssh root@"+IPB+" ifconfig "+ethB+" up";
			waitTime=(int)(LogNormalDistribution(failure_startTime_mu,failure_startTime_sigma)*meanA);
			usleep(waitTime);
			// down
			system(commandA.c_str());
			// system(commandB.c_str());
			if (!strcmp(ethA.c_str(),"eth0"))
				usleep(meanB);
			else 
				usleep(interval);
			// up
			system(commandC.c_str());
			// system(commandD.c_str());

			fseek(fp,0L,SEEK_END);
			fprintf(fp,(commandA+"\n").c_str());
			// fprintf(fp,(commandB+"\n").c_str());
			fprintf(fp,(commandC+"\n\n").c_str());
			// fprintf(fp,(commandD+"\n\n").c_str());
			fflush(fp);
		}
	}
}

int main(int argc, char const *argv[])
{
	if ((fp=fopen(argv[1],"a+"))==NULL)
	{
		printf("Cannot open record file.\n");
		exit(0);
	}
	
  	string str;
  	ifstream finA(argv[2],ios::app);

	interval=atoi(argv[3]);
	failure_startTime_mu=atof(argv[4]);
	failure_startTime_sigma=atof(argv[5]);
	meanA=atoi(argv[6]);
	meanB=atoi(argv[7]);
	string type=argv[8];

	pthread_t process_thread;

	vector<struct tempthreadparam> tempThreadParam;
	struct tempthreadparam tempParam;
	int linkNum=0;

	while (getline(finA,str))
	{
		tempParam.IPA=str.substr(0,str.length()-1);
		getline(finA,str);
		tempParam.IPB=str.substr(0,str.length()-1);
		getline(finA,str);
		tempParam.ethA=str.substr(0,str.length()-1);
		getline(finA,str);
		tempParam.ethB=str.substr(0,str.length()-1);
		tempParam.type=type;
		tempThreadParam.push_back(tempParam);
		linkNum++;
		getline(finA,str);// 读取空格
	}

	for (int i=0;i<linkNum;i++)
	{
		if (pthread_create(&process_thread,NULL,ProcessThread,(void*)(&tempThreadParam[i]))<0)
		{
			fseek(fp,0L,SEEK_END);
			fprintf(fp,"create thread for %s and %s failed.\n",tempThreadParam[i].IPA.c_str(),tempThreadParam[i].IPB.c_str());
			fflush(fp);
		}
	}

	start=true;
	pthread_exit(NULL);
	return 0;
}