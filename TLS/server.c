#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <dirent.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#define FAIL    -1
#define server_crt "./server.crt"
#define server_key "./server.key"
#define ca_crt "./ca.crt"

int listener(int port){
    int sd;
    struct sockaddr_in addr;
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ){
        perror("can't bind port");
        abort();
    }
    if ( listen(sd, 10) != 0 ){
        perror("Can't configure listening port");
        abort();
    }
    return sd;
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
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
}

void clientCert(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl);
    if ( cert != NULL ){
        printf("clinet certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("Client has no certificate.\n");
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
        clientCert(ssl);  
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
                    if(file=fopen(path, "r")){
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

int main(int argc, char *argv[]){

    SSL_CTX *ctx;
    int server;
    char *port;
    port = argv[1];

    ctx = InitServerCTX();

    if ( !SSL_CTX_load_verify_locations( ctx, ca_crt, NULL ) ) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    ck_cert(ctx, server_crt, server_key); 

    server = listener(atoi(port));

    while (1) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        SSL *ssl;
        int client = accept(server, (struct sockaddr*)&addr, &len);
        printf("Connection: %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);
        sreverProcess(ssl);
    }

    close(server);
    SSL_CTX_free(ctx);
}