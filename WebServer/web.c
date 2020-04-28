#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <sys/ioctl.h>

int main()
{
    FILE *file = fopen("form.html", "r");
	char output;
	while(fscanf(file,"%c", &output)==1)
		printf("%c", output);

	fclose(file);
}