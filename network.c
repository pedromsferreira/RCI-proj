#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "validation.h"
#include "network.h"

//default IP e UDP port
char defaultIP[16] = "193.136.138.142";
char defaultUDP[6] = "59000";

//Initialize socket for UDP connection
void UDP_socket(int argc, char *IP, char *UDP, int *sockfd)
{
    struct addrinfo hints;
    int flag;

    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sockfd == -1)
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
    return;
}

/*
Initialize socket for TCP connection with client
Return:
    fd if successfull
    -1 if something went wrong
*/
int TCP_client(int argc, char *IP, char *TCP, struct addrinfo *res)
{

    int sockfd, flag = 0;
    struct addrinfo hints;

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
        printf("Error: %s\n", gai_strerror(flag));
        return -1;
    }

    return sockfd;
}

void state_machine(int argc, char *argv[])
{
    //Variables
    int fd_ready, maxfd, sock_server, flag = 0;
    fd_set read_fd;

    //iniciar conexão ao servidor
    //preparar o socket UDP
    UDP_socket(argc, argv[3], argv[4], &sock_server);

    //entrar na máquina de estados como not registered
    state = notreg;

    printf("\nndn> ");
    fflush(stdout);

    while (state != exiting)
    {
        //limpar a estrutura
        FD_ZERO(&read_fd);
        //initialize states
        switch (state)
        {
        case notreg:
            FD_SET(0, &read_fd);
            maxfd = 0;
            break;
        case reg:
            break;
        case exiting:
            break;
        }

        //await for fds ready to be read
        fd_ready = select(maxfd + 1, &read_fd, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        //in case return value is wrong
        if (fd_ready <= 0)
        {
            printf("Error during select: %s\n", strerror(errno));
            //exit strat goes here
        }

        for (; fd_ready; fd_ready -= 1)
        {
            switch (state)
            {
            //for when program is not registered in the network
            case notreg:
                //if stdin (fd = 0) interreptud select
                if (FD_ISSET(0, &read_fd))
                {
                    FD_CLR(0, &read_fd);
                    flag = user_interface(sock_server, argv);
                    if (flag == -1)
                    {
                        printf("\nndn> ");
                        fflush(stdout);
                        continue;
                    }
                }
                break;
            case reg:
                //if stdin (fd = 0) interreptud select
                if (FD_ISSET(0, &read_fd))
                {
                    FD_CLR(0, &read_fd);
                    flag = user_interface(sock_server, argv);
                    if (flag == -1)
                    {
                        printf("\nndn> ");
                        fflush(stdout);
                        continue;
                    }
                }
                break;
            case exiting:
                
                break;
            }
        }
    }

    return;
}

//Espera por resposta do servidor no máximo 10s
int wait_for_answer(int sockfd)
{
    int counter = 0;
    fd_set read_fd;
    struct timeval tv;

    while (counter == 0)
    {
        FD_ZERO(&read_fd);
        FD_SET(sockfd, &read_fd);
        tv.tv_sec = 10;
        tv.tv_usec = 0;

        counter = select(sockfd + 1, &read_fd, (fd_set *)NULL, (fd_set *)NULL, &tv);
        if (counter <= 0)
        {
            printf("Timeout while waiting for server. Returning...\n");
            return -1;
        }
    }
    return 0;
}
/*
Pede a lista de nós do servidor de nós através do socket UDP
Return
    0 = good
    -1 = bad
*/
int ask_list(char *netID, int sockfd)
{
    int i = 0;
    char buffer[BUF_SIZE + 5] = "NODES ";
    char list[65536];
    char *token;

    struct sockaddr addr;
    socklen_t addrlen;

    strcat(buffer, netID); //buffer = NODES NETID

    if (sendto(sockfd, buffer, strlen(buffer) + 1, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    if(wait_for_answer(sockfd) == -1)
        return -1;

    if (recvfrom(sockfd, list, sizeof(list) + 1, 0, &addr, &addrlen) == -1)
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
    n_nodes = i;
    
    //print temporário do que a lista deu return
    for (int j = 0; j < i; j++)
    {
        printf("IP --> %s\n", nodeslist[j].IP);
        printf("TCP --> %s\n", nodeslist[j].TCP);
    }
    return 0;
}

