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

/******************************************************************************
*   Manages program when in following states (variable state in defines.h):
*       notreg - Not registered in nodes server
*       lonereg - Registered in nodes server but this node is the only one
*       reg - Registered with friends
*       leaving - intermediate state, only called when something bad happens
*       exiting - instructs program to close
*
* Arguments:
*   argc - number of arguments from program start
*   argv - arguments from program start
*
* Returns: (void)
*
******************************************************************************/
void state_machine(int argc, char **argv)
{

    //Variables
    int fd_ready, maxfd, sock_server, flag = 0, print_check = 1, i;
    int n_neighbours = 0; //número de vizinhos
    char netID[BUF_SIZE];
    struct timeval tv;
    
    fd_set read_fd;

    //Tabela de nós
    //Posições definidas:
    //  0 - próprio nó - não conta como vizinho
    //  1 - vizinho externo
    //  2 - vizinho de recuperação - não conta como vizinho
    //  3+ - vizinhos internos
    neighbour neighbours[MAX_NEIGHBOURS];

    //Tabela de expedição
    //Posições definidas: (ID)
    //  0 - próprio nó
    //  1+ - outros membros da rede
    expedition_table table;

    //Tabela de objetos
    //Posições definidas: (Cache)
    //  0 - posição mais recente
    //  1 - posição menos recente

    object_search FEDEX;

    tv.tv_sec = 3;
    tv.tv_usec = 0;

    //inicializar variáveis
    //Tabela de nós
    for (i = 0; i < MAX_NEIGHBOURS; i++)
    {
        memset(neighbours[i].mail_sent, '\0', BUF_SIZE * 4);
        neighbours[i].sockfd = -1;
    }

    //Tabela de expedição
    reset_table(&table);

    //Tabela de objetos
    reset_objects(&FEDEX);

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
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        //initialize states
        switch (state)
        {
        case notreg:
            FD_SET(0, &read_fd);
            maxfd = 0;
            break;
        case lonereg:
            FD_SET(0, &read_fd);
            for (i = 0; i <= n_neighbours + 1; i++)
            {
                if (neighbours[i].sockfd != -1)
                    FD_SET(neighbours[i].sockfd, &read_fd);
            }
            maxfd = max(neighbours, n_neighbours + 1);
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
            leave_protocol(netID,sock_server,argv,&n_neighbours,neighbours,&table,&FEDEX);
            state = notreg;
            break;
        case exiting:
            break;
        }

        if(print_check == 1)
        {
            printf("\nndn> ");
            fflush(stdout);
            print_check = 0;
        }

        //await for fds ready to be read
        fd_ready = select(maxfd + 1, &read_fd, (fd_set *)NULL, (fd_set *)NULL, &tv);

        //in case return value is wrong
        if (fd_ready < 0)
        {
            printf("Error during select: %s\n", strerror(errno));
            exit(1);
        }

        //verificar se algum objeto ainda está em trânsito
        if (fd_ready == 0)
        {
            check_clock(&FEDEX);
        }

        if (fd_ready > 0)
        {
            print_check = 1;
        }

        for (; fd_ready; fd_ready -= 1)
        {
            switch (state)
            {
            //for when program is not registered in the network
            case notreg:
                //if stdin (fd = 0) interrupted select
                if (FD_ISSET(0, &read_fd))
                {
                    FD_CLR(0, &read_fd);

                    //tratar do comando introduzido na consola
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID, &table, &FEDEX);
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
                //if stdin (fd = 0) interrupted select
                if (FD_ISSET(0, &read_fd))
                {
                    FD_CLR(0, &read_fd);

                    //tratar do comando introduzido na consola
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID, &table, &FEDEX);
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
                else if (FD_ISSET(neighbours[0].sockfd, &read_fd))
                {
                    FD_CLR(neighbours[0].sockfd, &read_fd);

                    //dar accept da conexão
                    n_neighbours++;
                    if(n_neighbours > 1)
                    {
                        neighbours[n_neighbours + 1].sockfd = accept_connection(neighbours[0].sockfd, neighbours[0]);
                        flag = neighbours[n_neighbours + 1].sockfd;
                    }
                    else if(n_neighbours == 1)
                    {
                        neighbours[n_neighbours].sockfd = accept_connection(neighbours[0].sockfd, neighbours[0]);
                        flag = neighbours[n_neighbours].sockfd;
                    }
                    
                    if (flag == -1)
                    {
                        n_neighbours--;
                        continue;
                    }
                    printf("\nNew node joining as your neighbour.\n");
                }
                //se for uma ligação dos nós já registados
                for (i = 1; i <= n_neighbours + 1; i++)
                {
                    if (FD_ISSET(neighbours[i].sockfd, &read_fd))
                    {
                        FD_CLR(neighbours[i].sockfd, &read_fd);
                        read_from_someone(neighbours, i, &n_neighbours, &table, &FEDEX);
                        //if everything OK -- state is now reg
                        state = reg;
                    }
                }
                break;
            case reg:
                //se for o stdin que deu trigger ao select
                if (FD_ISSET(0, &read_fd))
                {
                    FD_CLR(0, &read_fd);

                    //tratar do comando introduzido na consola
                    flag = user_interface(sock_server, argv, neighbours, &n_neighbours, netID, &table, &FEDEX);
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
                else if (FD_ISSET(neighbours[0].sockfd, &read_fd))
                {
                    FD_CLR(neighbours[0].sockfd, &read_fd);
                    //dar accept da conexão
                    n_neighbours++;
                    if(n_neighbours > 1)
                    {
                        neighbours[n_neighbours + 1].sockfd = accept_connection(neighbours[0].sockfd, neighbours[0]);
                        flag = neighbours[n_neighbours + 1].sockfd;
                    }
                    else if(n_neighbours == 1)
                    {
                        neighbours[n_neighbours].sockfd = accept_connection(neighbours[0].sockfd, neighbours[0]);
                        flag = neighbours[n_neighbours].sockfd;
                    }
                    
                    if (flag == -1)
                    {
                        n_neighbours--;
                        continue;
                    }
                    printf("\nNew node joining as your neighbour.\n");
                }
                //se for uma ligação dos nós já registados
                for (i = 1; i <= n_neighbours + 1; i++)
                {
                    if (FD_ISSET(neighbours[i].sockfd, &read_fd))
                    {
                        FD_CLR(neighbours[i].sockfd, &read_fd);
                        read_from_someone(neighbours, i, &n_neighbours, &table, &FEDEX);
                    }
                }
                break;
            case leaving:
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

/******************************************************************************
* Puts a timer in the connection estabilished (fd) for a certain amount of seconds
* Also leaves select open for other communications
*
* Returns: (int)
*   0 if timer hasn't been reached
*   -1 if timer is reached, making a timeout happen
******************************************************************************/
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


/******************************************************************************
* Inserts ID and socket in an available slot of the table struct
*
* Returns: (int)
*   0 if successfully put ID in table
*   -1 if ID is already in table
******************************************************************************/
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
/******************************************************************************
* Removes ID from various indexes of table struct, pushing table 1 line upwards every time a socket and ID are removed
******************************************************************************/
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
            memset(table->id[table->n_id], '\0', BUF_SIZE);
            table->sockfd[table->n_id] = -1;
            //Decrementar número de nós na rede
            table->n_id--;
        }
    }
    return;
}
/******************************************************************************
* Removes socket from various indexes of table struct, pushing table 1 line 
* upwards every time a socket and ID are removed
******************************************************************************/
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
            memset(table->id[table->n_id], '\0', BUF_SIZE);
            table->sockfd[table->n_id] = -1;
            //Decrementar número de nós na rede
            table->n_id--;
            i--;
        }
    }
    return;
}