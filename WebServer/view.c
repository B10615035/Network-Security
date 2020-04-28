#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <sys/ioctl.h>
#define PATH "localFile"

int main()
{
    FILE *fp = fopen(PATH, "r");
	char output;
	while(fscanf(fp,"%c", &output)==1)
	{
		printf("%c", output);
	}

	fclose(fp);
}