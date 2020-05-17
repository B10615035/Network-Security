#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define client_crt "./client.crt"
#define client_key "./client.key"
#define ca_crt "./ca.crt"


int Connect(const char *host, int port){
    int sd;
    struct hostent *Host;
    struct sockaddr_in addr;
    if ( (Host = gethostbyname(host)) == NULL )
    {
        perror(host);
        abort();
    }
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if ( connect(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        close(sd);
        perror(host);
        abort();
    }
    return sd;
}

SSL_CTX* InitCTX(void){
    SSL_METHOD *method;
    SSL_CTX *ctx;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = TLSv1_2_client_method();
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
}

void serverCert(SSL* ssl){
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl);
    if ( cert != NULL ){
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("Server has no certificate.\n");
}
int main(int argc, char *argv[])
{
    SSL_CTX *ctx;
    int server;
    SSL *ssl;
    char buf[1024];
    char acClientRequest[1024] = {0};
    int bytes;
    char *host, *port;
    
    host=argv[1];
    port=argv[2];

    ctx = InitCTX();

    if ( !SSL_CTX_load_verify_locations( ctx, ca_crt, NULL ) ) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    ck_cert(ctx, client_crt, client_key);
    
    server = Connect(host, atoi(port));
    ssl = SSL_new(ctx); 
    SSL_set_fd(ssl, server);
      

    if ( SSL_connect(ssl) == -1 )
        ERR_print_errors_fp(stderr);
    else {

        if(SSL_get_verify_result(ssl) == X509_V_OK)
            printf("verify success.\n");                
        else{
            printf("verify fail.\n");
            exit(1);
        }
        serverCert(ssl);    
        printf("\n\nConnected with %s encryption\n", SSL_get_cipher(ssl));
        
        int req=0;
        while (req==0){
            char command[100]={'\0'};
            printf("enter file name or \"list\" to ask server send available file list.\n");
            scanf("%s",command);
        
            SSL_write(ssl,command, strlen(command));
            bytes = SSL_read(ssl, buf, sizeof(buf));
            buf[bytes] = 0;

            if(strcmp(command,"list")==0){
                printf("%s\n", buf);
            }
            else{
                req=1;
                if(strcmp(buf,"No")==0)
                    printf("Can not find this file.\n");
                else {
                    FILE *file;
                    char name[10];
                    strcpy(name,command);
                    char path[20]="./ClientFile/";
                    strcat(path,name);

                    if(file=fopen(path,"a")){
                        for(int i=0; buf[i] != '\0'; i++)
                            fprintf(file, "%c", buf[i]);
                        fprintf(file, "\n");
                    }
                    printf("file copy success\n");
                }
            }
        }
        SSL_free(ssl);
    }
    close(server);
    SSL_CTX_free(ctx);
    return 0;
}