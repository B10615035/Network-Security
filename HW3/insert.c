#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <sys/ioctl.h>

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

	char path_c[50] = "ClientFile/";
	char path_s[50] = "ServerFile/";


	printf("File Name :\n");
	int ck = 0;
	for(int i= 0,j=10; buf[i] != '\0'; i++)
	{
        if(buf[i]=='='){
			ck = 1;
            continue;
        }
		if(ck){
			j++;
			printf("%c", buf[i]);
			path_s[j] = buf[i];
			path_c[j] = buf[i];
		}
	}
	
	FILE *file_s;
	char output;
	char s_file[500];
	int s_len = 0;
	if(file_s = fopen(path_s, "r")){
		while(fscanf(file_s, "%c", &output)==1){
			s_file[s_len++] =  output;
		}
		fclose(file_s);
		printf("\n\nFile Content:\n");
		FILE *file_c = fopen(path_c, "w");
		for(int i=0; s_file[i] != '\0'; i++)
		{
			fprintf(file_c, "%c", s_file[i]);
			printf("%c",s_file[i]);
		}
		printf("\n\nStore Success !\n");
		fclose(file_c);
	}
	else
	{
		printf("\n\nCan not find this file.");
	}
}