#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <sys/ioctl.h>

#define PATH "localFile"

int main()
{
	int unread, length;
	char *buf;

	while(unread < 1)
	{
		if(ioctl(STDIN_FILENO, FIONREAD, &unread))
		{
			perror("ioctl");
			exit(-1);
		}
	}

	buf = (char*)malloc(sizeof(char)*(unread+1));
	read(STDIN_FILENO, buf, unread);

    FILE *file = fopen(PATH, "a");
	

	int ck = 0;
	for(int i=0; buf[i] != '\0'; i++)
	{
        if(buf[i]=='='){
			ck=1;
            continue;
        }
		if(ck)
			fprintf(file, "%c", buf[i]);
	}
    fprintf(file, "\n");
    printf("Store Success !\n");
	fclose(file);
}