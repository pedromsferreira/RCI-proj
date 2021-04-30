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
#include "commands.h"
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

    //Tabela de expedição
    //Posições definidas:
    //  0 - próprio nó
    //  1+ - outros membros da rede
    expedition_table table;

    //inicializar variáveis
    //Tabela de nós
    for (i = 0; i < MAX_NEIGHBOURS; i++)
    {
        memset(neighbours[i].mail_sent, '\0', BUF_SIZE * 4);
        neighbours[i].sockfd = -1;
    }

    //Tabela de expedição
    reset_table(&table);

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
            for (i = 0; i <= n_neighbours; i++)
            {
                if (neighbours[i].sockfd != -1)
                    FD_SET(neighbours[i].sockfd, &read_fd);
            }
            maxfd = max(neighbours, n_neighbours);
            break;
        case reg:
            FD_SET(0, &read_fd);
            for (i = 0; i <= n_neighbours + 1; i++)
            {
                if (neighbours[i].sockfd != -1)
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
        if (state != leaving)
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
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID, &table);
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
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID, &table);
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
                    if (neighbours[n_neighbours].sockfd == -1)
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
                    read_from_someone(neighbours, 1, &n_neighbours, &table);
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
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID, &table);
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
                    neighbours[n_neighbours + 1].sockfd = accept_connection(neighbours[0].sockfd, neighbours[0]);
                    if (neighbours[n_neighbours + 1].sockfd == -1)
                    {
                        n_neighbours--;
                        continue;
                    }
                    printf("New node joining the network.\n");
                }
                //se for uma ligação dos nós já registados
                for (i = 1; i <= n_neighbours + 1; i++)
                {
                    if (FD_ISSET(neighbours[i].sockfd, &read_fd))
                    {
                        FD_CLR(neighbours[i].sockfd, &read_fd);
                        read_from_someone(neighbours, i, &n_neighbours, &table);
                    }
                }
                break;
            case leaving:
                //if stdin (fd = 0) interrupted select
                if (FD_ISSET(0, &read_fd))
                {
                    FD_CLR(0, &read_fd);

                    //tratar do comando introduzido na consola
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID, &table);
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

/*
Inserts specified id in table with respective fd
Return:
    0 if didn't have information in table and filled it
    -1 if already in table
WARNING: n_neighbours does not count with previous EXTERN
*/
int insert_ID_in_table(expedition_table *table, int sockfd, char *ID)
{

    for (int i = 0; i < table->n_id; i++)
    {
        //Se corresponde ao ID
        if (strcmp(table->id[i], ID) == 0)
        {
            return -1;
        }
    }
    //Copiar valores passados na função para a tabela
    strcpy(table->id[table->n_id], ID);
    table->sockfd[table->n_id] = sockfd;

    //Incrementar número de nós na rede
    table->n_id++;

    return 0;
}

void remove_ID_from_table(expedition_table *table, char *ID)
{
    //Retirar valores passados na função da tabela
    for (int i = 0; i < table->n_id; i++)
    {
        //Se corresponde ao ID
        if (strcmp(table->id[i], ID) == 0)
        {
            //Dar reset à informação da tabela no índice i
            memset(table->id[i], '\0', BUF_SIZE);
            table->sockfd[i] = -1;

            for (int j = i + 1; j < table->n_id; j++)
            {
                //Puxar tabela 1 linha para cima
                strcpy(table->id[j - 1], table->id[j]);
                table->sockfd[j - 1] = table->sockfd[j];
            }
            //Decrementar número de nós na rede
            table->n_id--;
        }
    }
    return;
}

void remove_socket_from_table(expedition_table *table, int sockfd)
{
    //Retirar valores passados na função da tabela
    for (int i = 0; i < table->n_id; i++)
    {
        //Se corresponde ao socket indicado
        if (table->sockfd[i] == sockfd)
        {
            //Dar reset à informação da tabela no índice i
            memset(table->id[i], '\0', BUF_SIZE);
            table->sockfd[i] = -1;

            for (int j = i + 1; j < table->n_id; j++)
            {
                //Puxar tabela 1 linha para cima
                strcpy(table->id[j - 1], table->id[j]);
                table->sockfd[j - 1] = table->sockfd[j];
            }
            //Decrementar número de nós na rede
            table->n_id--;
            i--;
        }
    }
    return;
}
