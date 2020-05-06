#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#define PORT 6789 
#define MAXLINE 1024 
void fatal(char *msg)
{
	perror(msg);
	exit(1);
}
int main()
{ 
    int sockfd; 
    char buffer[MAXLINE]; 
    struct sockaddr_in server_addr,client_addr; 
    if((sockfd=socket(AF_INET,SOCK_DGRAM,0))==-1) 
        fatal("socket");   
    memset(&server_addr,0,sizeof(server_addr)); 
    memset(&client_addr,0,sizeof(client_addr)); 
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY; 
    server_addr.sin_port=htons(PORT); 
    if(bind(sockfd,(const struct sockaddr *)&server_addr,sizeof(server_addr))==-1) 
        fatal("bind");
    int dummy,n;  
    dummy=sizeof(client_addr);
    while(1)
    {
        n=recvfrom(sockfd,(char *)buffer,MAXLINE,MSG_WAITALL,(struct sockaddr *)&client_addr,&dummy); 
        buffer[n]='\0';
        const char key[]={0xB7,0xDA,0xE6,0xB0,0x56,0xBC,0xC6,0x75,0x55,0x6A,0x40,0x5F,0xA5,0xED,0xA5,0x6B};
        const char iv[]= {0x50,0xF4,0xF8,0xF0,0x80,0x83,0x3C,0xAB,0x39,0x4B,0x9A,0x58,0xBB,0x9D,0x81,0x64};
        //key was generated by running
        //openssl enc -pbkdf2 -aes-128-cbc -k interstellarkey -P
        unsigned char *plaintext;
        plaintext=(unsigned char *)malloc(128);
        EVP_CIPHER_CTX *ctx;//create cipher context
        ctx=EVP_CIPHER_CTX_new();//init context
        EVP_DecryptInit_ex(ctx,EVP_aes_128_cbc(),NULL,(unsigned char *)key,(unsigned char *)iv);//init the decryption operation
        int len;
        EVP_DecryptUpdate(ctx,plaintext,&len,buffer,n);//decrypt n bytes
        EVP_DecryptFinal_ex(ctx,plaintext+len,&len);//finalize the decryption process, pad if needed
        printf("Client: %s\n",plaintext);
        EVP_CIPHER_CTX_cleanup(ctx);
        free(plaintext);
    } 
    return 0; 
} 
