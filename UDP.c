#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "defines.h"
#include "network.h"
#include "UDP.h"

//default IP e UDP port
char defaultIP[16] = "193.136.138.142";
char defaultUDP[6] = "59000";

//Initialize UDP server connection
int UDP_socket(int argc, char *IP, char *UDP)
{
    struct addrinfo hints;
    int flag, sockfd;


    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        printf("Error: %s\n", strerror(errno));
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (argc == 3)
        flag = getaddrinfo(defaultIP, defaultUDP, &hints, &server_info);
    else
        flag = getaddrinfo(IP, UDP, &hints, &server_info);

    if (flag != 0)
    {
        printf("Error: %s\n", gai_strerror(flag));
        exit(1);
    }
    return sockfd;
}

/*
Pede a lista de nós do servidor de nós através do socket UDP
Return
    0 = good
    -1 = bad
*/
int ask_list(char *netID, int sockfd, nodes* nodeslist, int* n_nodes)
{
    int i = 0;
    char buffer[BUF_SIZE + 5] = "NODES ";
    char list[65536];
    char *token;

    /*struct sockaddr addr;
    socklen_t addrlen;*/

    strcat(buffer, netID); //buffer = NODES NETID

    if (sendto(sockfd, buffer, strlen(buffer) + 1, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    if(wait_for_answer(sockfd, 2) == -1)
        return -1;

    //addrlen = sizeof(addr);
    if (recvfrom(sockfd, list, sizeof(list) + 1, 0, NULL, NULL) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    //Lê a listlista de nodes que vem do servidor
    token = strtok(list, "\n");
    token = strtok(NULL, "\n");
    while (token != NULL)
    {
        sscanf(token, "%s %s", nodeslist[i].IP, nodeslist[i].TCP);
        i++;
        token = strtok(NULL, "\n");
    }

    //Guardar número total de nós
    *n_nodes = i;
    
    //print temporário do que a lista deu return
    for (int j = 0; j < i; j++)
    {
        printf("IP --> %s\n", nodeslist[j].IP);
        printf("TCP --> %s\n", nodeslist[j].TCP);
    }
    return 0;
}
