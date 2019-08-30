#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main ()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	FILE * fp;
	if ((fp=fopen("/home/guolab/output/updateKernelTimeRecord.txt","a+"))==NULL)
	{
		printf("Cannot open updateKernelTimeRecord.txt\n");
		exit(0);
	}

	fseek(fp,0L,SEEK_END);
    fprintf(fp, "%f\n",tv.tv_sec+tv.tv_usec*0.000001);
    fflush(fp);
    fclose(fp);
    return 0;
}
