#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <dirent.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <err.h>
#define FAIL    -1
#define server_crt "./server.crt"
#define server_key "./server.key"
#define ca_crt "./ca.crt"

char buffer[5000];

void ck_cert(SSL_CTX* ctx, char* CertFile, char* KeyFile){
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    if ( !SSL_CTX_check_private_key(ctx) ){
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

SSL_CTX* InitServerCTX(void){
    SSL_METHOD *method;
    SSL_CTX *ctx;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = TLSv1_2_server_method();
    ctx = SSL_CTX_new(method);
    if ( ctx == NULL ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void sreverProcess(SSL* ssl)
{   
    char buf[1024] = {0};
    int sd, bytes;

    if ( SSL_accept(ssl) == FAIL ){
        ERR_print_errors_fp(stderr);
    } else {
        
        if(SSL_get_verify_result(ssl) == X509_V_OK)
            printf("verify success.\n");                
        else{
            printf("verify fail.\n");
            exit(1);
        }
        int req=0;
        while (req==0){
            bytes = SSL_read(ssl, buf, sizeof(buf));
            buf[bytes] = '\0';
            printf("Client message: \"%s\"\n", buf);
            if ( bytes > 0 ){
                if(strcmp("list",buf) == 0){
                    DIR *d;
                    struct dirent *dir;
                    char list[200]="\n";
                    d = opendir("./ServerFile");
                    if (d) {
                        while ((dir = readdir(d)) != NULL) {
                            if(strcmp(dir->d_name,"..")!=0&&strcmp(dir->d_name,".")!=0){
                                strcat(list,dir->d_name);
                                strcat(list,"\n");
                            } 
                        }
                        closedir(d);
                    }
                    SSL_write(ssl, list, strlen(list));
                } else {
                    req=1;
                    FILE *file;
                    char name[10];
                    strcpy(name,buf);
                    char path[20]="./ServerFile/";
                    strcat(path,name);
                    if(file=fopen(path,"r")){
                        char *output;
                        char result[100]={NULL};
                        int len=0;
                        while(fscanf(file,"%c", &output)==1){
                            result[len]=output;
                            len++;
                        }
                        SSL_write(ssl, result, strlen(result));
                    } else {
                        SSL_write(ssl, "No", strlen("No"));
                    }
                }
            }
            else
            {
                ERR_print_errors_fp(stderr);
            }
        }
    }
    sd = SSL_get_fd(ssl);
    SSL_free(ssl); 
    close(sd);
}



void CGI(SSL *client_fd, char* router, char* method, char* post_msg)
{
	sprintf(buffer, "HTTP/1.1 200 OK\r\n\r\n");
	SSL_write(client_fd, buffer, strlen(buffer));

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
			SSL_write(client_fd, &c, 1);
		close(cgiOutput[0]);
		close(cgiInput[1]);
		waitpid(cpid, &status, 0);	
	}
}
void pre(SSL *client_fd)
{
	SSL_read(client_fd, buffer, 5000);
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
    svr_addr.sin_port = htons(8080);


	int sock;
	if((sock = socket(AF_INET , SOCK_STREAM , 0)) < 0)
		err(1, "can't open socket");

	if((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) < 0)
	{
		perror("setsockopt error");
		exit(-1);
	}


	if(bind(sock,(struct sockaddr *)&svr_addr,sizeof(svr_addr)) == -1)
	{
		perror("bind error");
		exit(-1);
	}


	listen(sock, 5);

	while(1)
	{
        printf("connect\n");

		struct sockaddr_in cli_addr;
		socklen_t clientLen = sizeof(cli_addr);
        SSL_CTX *ctx;

        ctx = InitServerCTX();

        if ( !SSL_CTX_load_verify_locations( ctx, ca_crt, NULL ) ) {
            ERR_print_errors_fp(stderr);
            exit(1);
        }

        ck_cert(ctx, server_crt, server_key); 

        SSL *ssl;
		int client_fd = accept(sock, (struct sockaddr*)&cli_addr, &clientLen);

		if(client_fd == -1)
		{
			printf("error0\n");
			perror("accept client socket error");
			exit(-1);
		}

		ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);
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
                printf("child\n");
                 if ( SSL_accept(ssl) == FAIL ){
                    ERR_print_errors_fp(stderr);
                } else {
                    if(SSL_get_verify_result(ssl) == X509_V_OK)
                        printf("verify success.\n");                
                    else{
                        printf("verify fail.\n");
                        exit(1);
                    }
                    close(sock);
                    pre(ssl);
                }
			}
			else			// parent process
			{
                printf("parent\n");
				close(client_fd);
			}
		}
	}
	close(sock);
	exit(0);
}