#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "defines.h"
#include "TCP.h"


/*
Initialize socket for TCP connection with client
Return:
    fd if successfull
    -1 if something went wrong
*/
int TCP_client(char* IP, char* TCP, struct addrinfo* node_info)
{

    int sockfd, flag = 0;
    struct addrinfo hints;
    struct addrinfo *res;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    flag = getaddrinfo(IP, TCP, &hints, &res);
    if (flag != 0)
    {
        printf("Error: %s\n", gai_strerror(flag));
        return -1;
    }

    flag = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (flag != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    node_info = res;
    return sockfd;
}

int TCP_server(char* TCP, struct addrinfo* server_addr)
{
    int sockfd, flag;
    struct addrinfo hints;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE;

    flag = getaddrinfo(NULL, TCP, &hints, &server_addr);
    if (flag != 0)
    {
        printf("Error: %s\n", gai_strerror(flag));
        return -1;
    }

    flag = bind(sockfd, server_addr->ai_addr, server_addr->ai_addrlen);
    if (flag != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    flag = listen(sockfd,5);
    if (flag != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }


    return sockfd;
}

/*
Send a message through TCP
Return:
    0 if successfull
    -1 if something went wrong
*/
int write_to_someone(char* nodeIP, char* nodeTCP, int nodefd, char* command)
{
    int left = 0, flag = 0;
    char *ptr, buffer[BUF_SIZE];

    sprintf(buffer, "%s %s %s\n", command, nodeIP, nodeTCP);
    left = strlen(buffer);
    ptr = buffer;

    while(left > 0)
    {
        flag = write(nodefd, ptr, left);
        //erro na escrita ou closed by peer
        if(flag == -1 || flag == 0)
        {
            return -1;
        }

        left -= flag;
        ptr += flag;
    }
    //enviado com sucesso
    return 0;
}

/*
Send a message through TCP
Return:
    0 if successfull
    -1 if something went wrong
*/
int read_join(neighbour* placeholder)
{
    int received = 1, total = 0;
    char *ptr;

    //Pointer no início da string
    ptr = placeholder->mail;
    
    while(received != 0)
    {
        received = read(placeholder->sockfd, ptr, BUF_SIZE*4);
        
        ptr += received;
        total += received;
        if(received == -1)
            return -1;
    }
    
    //Identificar se mensagem está completa ou não
    
    //Identificar se mensagem é lixo ou não
    

    
    //enviado com sucesso
    return 0;
}

