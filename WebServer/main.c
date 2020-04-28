#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>

char buffer[5000];
void CGI(int client_fd, char* router, char* method, char* post_msg)
{
	sprintf(buffer, "HTTP/1.1 200 OK\r\n\r\n");
	write(client_fd, buffer, strlen(buffer));

	int cgiInput[2], cgiOutput[2];
	int status;
    pid_t cpid;
	char c;

    if(pipe(cgiInput)<0){
            perror("pipe");
            exit(EXIT_FAILURE);
      }
      if(pipe(cgiOutput)<0){
            perror("pipe");
            exit(EXIT_FAILURE);
      }

      if((cpid = fork()) < 0){
            perror("fork");
            exit(EXIT_FAILURE);
      }

	if(cpid == 0) {	
		close(cgiInput[1]);
		close(cgiOutput[0]);
		dup2(cgiInput[0], STDIN_FILENO);
		dup2(cgiOutput[1], STDOUT_FILENO);
		close(cgiInput[0]);
		close(cgiOutput[1]);
		execlp(router, router, NULL);
		exit(0);
	}
	else {
		close(cgiOutput[1]);
		close(cgiInput[0]);
		write(cgiInput[1], post_msg, strlen(post_msg));
		while(read(cgiOutput[0], &c, 1) > 0)
			write(client_fd, &c, 1);
		close(cgiOutput[0]);
		close(cgiInput[1]);
		waitpid(cpid, &status, 0);	
	}
}
void pre(int client_fd)
{
	read(client_fd, buffer, 5000);
    printf("%s", buffer);

	int i, j;
	char method[10], url[10];
	for(i = 0, j = 0; buffer[j] != ' '; i++, j++)
		method[i] = buffer[j];

	method[i] = '\0';

	while(buffer[j] == ' ' && j < sizeof(buffer))
		j++;

	for(i = 0; buffer[j] != ' '; i++, j++)
		url[i] = buffer[j];
	url[i] = '\0';

	char post_msg[100];
	if(strcmp(method, "POST") == 0 || strcmp(method, "post") == 0)
	{
		char *delim = ": \r\n";
		char *temp = strtok(buffer, delim);
		int n = 0;
		while(temp != NULL)
		{
			strcpy(post_msg, temp);
			temp = strtok(NULL, delim);
		}
	}

	char *path;
	if(strcmp(url, "/view") == 0)
		path="./view.cgi\0";
	else if(strcmp(url, "/") == 0 || strcmp(url, "/insert") == 0)
		path="./web.cgi\0";
	else if(strcmp(url, "/success") == 0)
		path="./insert.cgi\0";

	CGI(client_fd, path, method, post_msg);
}


int main(int argc , char *argv[])
{
	
	struct sockaddr_in svr_addr;
    int one=1;
	svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(8787);

	// socket building
	int sock;
	if((sock = socket(AF_INET , SOCK_STREAM , 0)) < 0)
		err(1, "can't open socket");

	if((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) < 0)
	{
		perror("setsockopt error");
		exit(-1);
	}

	// socket connection
	if(bind(sock,(struct sockaddr *)&svr_addr,sizeof(svr_addr)) == -1)
	{
		perror("bind error");
		exit(-1);
	}

	// listen queue
	listen(sock, 5);

	while(1)
	{
        printf("connect\n");
		struct sockaddr_in cli_addr;
		socklen_t clientLen = sizeof(cli_addr);

		// wait for request from client
		int client_fd = accept(sock, (struct sockaddr*)&cli_addr, &clientLen);
		if(client_fd == -1)
		{
			perror("accept client socket error");
			exit(-1);
		}
		
		int pid = fork();
		if(pid < 0)
		{
			perror("fork error");
			exit(-1);
		}
		else
		{
			if(pid == 0)	// child process
			{
				close(sock);
				pre(client_fd);
			}
			else			// parent process
			{
				close(client_fd);
			}
		}
	}
	close(sock);
	exit(0);
}

