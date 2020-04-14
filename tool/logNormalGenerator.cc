#include<math.h>
#include<stdlib.h>
#include<stdio.h>
#include <time.h>
#define pi 3.1415926535

double UniformDistribution(double min, double max);
double NormalDistribution(double mu,double sigma);
double LogNormalDistribution(double mu,double sigma);

int main(int argc, char **argv)
{
  if(argc!=4)
  {
    printf("usage: failure_startTime_mu, failure_startTime_sigma, totalFailureNumber");
    exit(0);
  }
  double failure_startTime_mu, failure_startTime_sigma;
  failure_startTime_mu=strtod(argv[1],NULL);
  failure_startTime_sigma=strtod(argv[2],NULL);
  int totalFailureNumber=atoi(argv[3]);
  
  double failure_startTime;
  for(int i=0;i<totalFailureNumber;i++) {
    failure_startTime=LogNormalDistribution(failure_startTime_mu,failure_startTime_sigma);
    printf("%d\n",int(failure_startTime*10000));
  }
    
  return 0;
}



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

