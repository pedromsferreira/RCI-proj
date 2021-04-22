#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "defines.h"
#include "validation.h"
#include "network.h"
#include "UDP.h"
#include "TCP.h"

//Gestão dos estados do programa
void state_machine(int argc, char *argv[])
{
    //Variables
    int fd_ready, maxfd, sock_server, flag = 0;
    fd_set read_fd;
    nodes topology[2]; //1a coluna --> vizinho externo || 2a coluna --> vizinho de recuperação
    nodes nodeslist[MAX_NODES];

    //iniciar conexão ao servidor
    //preparar o socket UDP
    sock_server = UDP_socket(argc, argv[3], argv[4]);

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
                    flag = user_interface(sock_server, argv, topology, nodeslist);
                    if (flag == -1)
                    {
                        printf("\nndn> ");
                        fflush(stdout);
                        continue;
                    }
                }
                break;
            case reg:
                
                break;
            case exiting:
                
                break;
            }
        }
    }
    close(sock_server);
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

