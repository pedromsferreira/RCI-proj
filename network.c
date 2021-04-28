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
void state_machine(int argc, char **argv)
{
    
    //Variables
    int fd_ready, maxfd, sock_server, flag = 0, i;
    int n_neighbours = 0; //número de vizinhos
    char netID[BUF_SIZE];
    fd_set read_fd;

    //Tabela de nós
    //Posições definidas:
    //  0 - próprio nó - não conta como vizinho
    //  1 - vizinho externo
    //  2 - vizinho de recuperação - não conta como vizinho
    //  3+ - vizinhos internos
    neighbour neighbours[MAX_NEIGHBOURS];

    //expedition_table table;

    //inicializar variáveis
    for(i = 0; i < MAX_NEIGHBOURS; i++)
    {
        memset(neighbours[i].mail_sent, '\0', BUF_SIZE*4);
        neighbours[i].sockfd = -1;
    }

    //Establecer neighbour 0 como o programa ndn
    strcpy(neighbours[0].node.IP, argv[1]);
    strcpy(neighbours[0].node.TCP, argv[2]);

    //iniciar conexão ao servidor
    //preparar o socket UDP
    sock_server = UDP_socket(argc, argv[3], argv[4]);

    //entrar na máquina de estados como not registered
    state = notreg;

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
        case lonereg:
            FD_SET(0, &read_fd);
            for(i = 0; i <= n_neighbours; i++)
            {
                if(neighbours[i].sockfd != -1)
                    FD_SET(neighbours[i].sockfd, &read_fd);
            }
            maxfd = max(neighbours, n_neighbours);
            break;
        case reg:
            FD_SET(0, &read_fd);
            for(i = 0; i <= n_neighbours + 1; i++)
            {
                if(neighbours[i].sockfd != -1)
                    FD_SET(neighbours[i].sockfd, &read_fd);
            }
            maxfd = max(neighbours, n_neighbours + 1);
            break;
        case leaving:
            FD_SET(0, &read_fd);
            maxfd = 0;
            break;
        case exiting:
            break;
        }

        printf("\nndn> ");
        if(state != leaving)
            fflush(stdout);

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

                    //tratar do comando introduzido na consola
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID/*, &table*/);
                    if (flag == 1)
                    {
                        printf("Closing program...\n");
                        continue;
                    }
                    
                    if (flag == -1)
                    {
                        continue;
                    }
                }
                break;
            case lonereg:
                //if stdin (fd = 0) interreptud select
                if (FD_ISSET(0, &read_fd))
                {
                    FD_CLR(0, &read_fd);

                    //tratar do comando introduzido na consola
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID/*, &table*/);
                    if (flag == 1)
                    {
                        printf("Closing program...\n");
                        continue;
                    }
                    if (flag == -1)
                    {
                        continue;
                    }
                }
                //se for o listen fd que deu trigger ao select
                if (FD_ISSET(neighbours[0].sockfd, &read_fd))
                {
                    FD_CLR(neighbours[0].sockfd, &read_fd);

                    //dar accept da conexão
                    n_neighbours++;
                    neighbours[n_neighbours].sockfd = accept_connection(neighbours[0].sockfd, neighbours[0]);
                    if(neighbours[n_neighbours].sockfd == -1)
                    {
                        n_neighbours--;
                        continue;

                    }
                    printf("New node joining the network.\n");
                }
                //se for o vizinho externo que deu trigger ao select
                if (FD_ISSET(neighbours[1].sockfd, &read_fd))
                {
                    FD_CLR(neighbours[1].sockfd, &read_fd);
                    //ler buffer
                    read_from_someone(neighbours, 1, &n_neighbours);
                    //if everything OK -- state is now reg
                    state = reg;
                }
                break;
            case reg:
                //if stdin (fd = 0) interreptud select
                if (FD_ISSET(0, &read_fd))
                {
                    FD_CLR(0, &read_fd);

                    //tratar do comando introduzido na consola
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID/*, &table*/);
                    if (flag == 1)
                    {
                        printf("Closing program...\n");
                        continue;
                    }
                    if (flag == -1)
                    {
                        continue;
                    }
                }
                //se for o listen fd que deu trigger ao select
                if (FD_ISSET(neighbours[0].sockfd, &read_fd))
                {
                    FD_CLR(neighbours[0].sockfd, &read_fd);
                    //dar accept da conexão
                    n_neighbours++;
                    neighbours[n_neighbours+1].sockfd = accept_connection(neighbours[0].sockfd, neighbours[0]);
                    if(neighbours[n_neighbours+1].sockfd == -1)
                    {
                        n_neighbours--;
                        continue;
                    }
                    printf("New node joining the network.\n");
                }
                //se for uma ligação dos nós já registados
                for(i = 1; i <= n_neighbours + 1; i++)
                {
                    if(FD_ISSET(neighbours[i].sockfd, &read_fd))
                    {
                        FD_CLR(neighbours[i].sockfd, &read_fd);
                        read_from_someone(neighbours, i, &n_neighbours);
                    }
                }
                break;
            case leaving:
                //if stdin (fd = 0) interreptud select
                if (FD_ISSET(0, &read_fd))
                {
                    FD_CLR(0, &read_fd);

                    //tratar do comando introduzido na consola
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID/*, &table*/);
                    if (flag == 1)
                    {
                        printf("Closing program...\n");
                        continue;
                    }
                    
                    if (flag == -1)
                    {
                        continue;
                    }
                }
                break;
            case exiting:
                break;
            }
        }
    }
    //fechar resto dos sockets
    close(sock_server); //UDP
    freeaddrinfo(server_info);
    return;
}

//Espera por resposta do fd por segundos determinados
int wait_for_answer(int sockfd, int seconds)
{
    int counter = 0;
    fd_set read_fd;
    struct timeval tv;

    FD_ZERO(&read_fd);
    FD_SET(sockfd, &read_fd);
    tv.tv_sec = seconds;
    tv.tv_usec = 0;

    counter = select(sockfd + 1, &read_fd, (fd_set *)NULL, (fd_set *)NULL, &tv);
    if (counter <= 0)
    {
        printf("Timeout while waiting. Returning...\n");
        return -1;
    }

    return 0;
}

