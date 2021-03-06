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
#include "validation.h"
#include "commands.h"
#include "network.h"
#include "TCP.h"

/******************************************************************************
* Initialize socket for TCP connection with client
*
* Returns: (int)
*   fd if successfull
*   -1 if something went wrong
******************************************************************************/
int TCP_client(char *IP, char *TCP, struct addrinfo *node_info)
{

    int sockfd, flag = 0 /*, option = 1*/;
    struct addrinfo hints;
    struct addrinfo *res;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    //Para se poder reutilizar o socket instantaneamente
    //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

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
    freeaddrinfo(res);
    return sockfd;
}

/******************************************************************************
* Initialize socket for listening other clients
*
* Returns: (int)
*   fd if successfull
*   -1 if something went wrong
******************************************************************************/
int TCP_server(char *TCP, neighbour *neighbours)
{
    int sockfd, flag /*, option = 1*/;
    struct addrinfo hints;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        return -1;
    }
    //Para se poder reutilizar o socket instantaneamente
    //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    flag = getaddrinfo(NULL, TCP, &hints, &neighbours[0].node_info);
    if (flag != 0)
    {
        printf("Error: %s\n", gai_strerror(flag));
        return -1;
    }

    flag = bind(sockfd, neighbours[0].node_info->ai_addr, neighbours[0].node_info->ai_addrlen);
    if (flag != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    flag = listen(sockfd, 5);
    if (flag != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    return sockfd;
}

/******************************************************************************
* Send a message through TCP
*
* Arguments:
*   argument1, argument2 - use both for topology, insert "0" in argument2 for 
*   routing and search for object
*
* Returns: (int)
*    0 if successfull
*   -1 if something went wrong
******************************************************************************/
int write_to_someone(char *argument1, char *argument2, neighbour *neighbours, char *command, int destination, int *n_neighbours, expedition_table *table, object_search *FEDEX)
{
    int left = 0, flag = 0;
    char *ptr, buffer[BUF_SIZE];

    memset(buffer, '\0', BUF_SIZE);

    //Comando de protocolo de topologia
    if (strcmp(argument2, "0") != 0)
        sprintf(buffer, "%s %s %s\n", command, argument1, argument2);

    //Comando de protocolo de encaminhamento e de pesquisa de objeto
    if (strcmp(argument2, "0") == 0)
        sprintf(buffer, "%s %s\n", command, argument1);

    left = strlen(buffer);
    ptr = buffer;

    while (left > 0)
    {
        flag = write(neighbours[destination].sockfd, ptr, left);
        //erro na escrita
        if (flag == -1)
        {
            backup_plan(destination, n_neighbours, neighbours, table, FEDEX);
            return -1;
        }

        left -= flag;
        ptr += flag;
    }
    //enviado com sucesso
    return 0;
}

/******************************************************************************
* Re-route message to execute related protocol
*
*   known flags:
*       1 - NEW
*       2 - EXTERN
*       3 - ADVERTISE
*       4 - WITHDRAW 
*       5 - INTEREST
*       6 - DATA
*       7 - NODATA
******************************************************************************/
void TCP_command_hub(int flag, neighbour *neighbours, char *mail, int *n_neighbours, int ready_index, expedition_table *table, object_search *FEDEX)
{
    switch (flag)
    {
    case 1:
        execute_NEW(neighbours, mail, n_neighbours, ready_index, table);
        break;

    case 2:
        execute_EXTERN(neighbours, mail, ready_index);
        break;

    case 3:
        execute_ADVERTISE(neighbours, mail, ready_index, table, n_neighbours);
        break;

    case 4:
        execute_WITHDRAW(neighbours, mail, ready_index, table, n_neighbours);
        break;

    case 5:
        execute_INTEREST(neighbours, mail, ready_index, table, n_neighbours, FEDEX);
        break;

    case 6:
        execute_DATA(neighbours, mail, ready_index, table, n_neighbours, FEDEX);
        break;

    case 7:
        execute_NODATA(neighbours, mail, ready_index, table, n_neighbours, FEDEX);
        break;
    }

    return;
}

/******************************************************************************
* Read a message through TCP
*
* Returns: (int)
*   0 if normal procedure
*   -1 if something went wrong
******************************************************************************/
int read_from_someone(neighbour *placeholder, int ready_index, int *n_neighbours, expedition_table *table, object_search *FEDEX)
{
    int received = 0, flag = 0;
    char *ptr, *ptr2;
    char buffer[BUF_SIZE*4];

    //memset(buffer, '\0', BUF_SIZE * 4);

    //Pointer no in??cio da string
    ptr = placeholder[ready_index].mail_sent;
    ptr2 = placeholder[ready_index].mail_sent;

    //Guardar mensagem na struct, tendo em conta se j?? tem l?? informa????o
    received = read(placeholder[ready_index].sockfd, ptr + strlen(placeholder[ready_index].mail_sent), BUF_SIZE * 4);

    //socket fechou ou algo de mal aconteceu
    if (received == 0 || received == -1)
    {
        backup_plan(ready_index, n_neighbours, placeholder, table, FEDEX);
        return 0;
    }

    //buffer overflow
    if (strlen(placeholder[ready_index].mail_sent) >= BUF_SIZE * 4 - 1 && strstr(placeholder[ready_index].mail_sent, "\n") == NULL)
    {
        memset(placeholder[ready_index].mail_sent, '\0', BUF_SIZE * 4);
        return 0;
    }

    //for debugging only
    //printf("\nMessage received:\n%s",placeholder[ready_index].mail_sent);

    //Encontrar "\n" na mensagem
    if (received > 0)
    {
        while (1) //Enquanto houver "\n", estar?? sempre ?? procura de mais comandos
        {
            ptr2 = strstr(placeholder[ready_index].mail_sent, "\n");
            //mensagem incompleta
            if (ptr2 == NULL)
            {
                return 0;
            }

            //Validar o argumento
            flag = validate_messages(placeholder[ready_index].mail_sent);

            //Executar o comando
            TCP_command_hub(flag, placeholder, placeholder[ready_index].mail_sent, n_neighbours, ready_index, table, FEDEX);

            //Atualiza a string para estar ?? frente do "\n"
            ptr2 += 1;
            strcpy(buffer, ptr2);
            //memset(placeholder[ready_index].mail_sent, '\0', BUF_SIZE * 4); //clean buffer for copy
            strcpy(placeholder[ready_index].mail_sent, buffer);
        }
    }

    //you shouldn't be here
    return -1;
}


/******************************************************************************
* Accept a connection from client through TCP
*
* Returns: (int)
*   fd if normal procedure
*   -1 if connection is missing
******************************************************************************/
int accept_connection(int listenfd, neighbour neighbours)
{
    int newfd;

    neighbours.node_info->ai_addrlen = sizeof(neighbours.node_info->ai_addr);
    newfd = accept(listenfd, neighbours.node_info->ai_addr, &neighbours.node_info->ai_addrlen);
    if (newfd == -1)
    {
        return -1;
    }

    return newfd;
}

/******************************************************************************
* Promotes internal neighbour to EXTERN
*
*   Returns: (int)
*   0 if someone is promoted
*   -1 if there's no neighbours able to be promoted
*   WARNING: n_neighbours does not count with previous external neighbour
******************************************************************************/
int promote_to_EXTERN(neighbour *neighbours, int *n_neighbours, expedition_table *table, object_search *FEDEX)
{
    int i;
    //Se tem internos
    printf("\nChecking if there's an available internal neighbour for connection...\n");
    if (*n_neighbours > 0)
    {
        while (*n_neighbours > 0)
        {
            neighbours[1] = neighbours[3];
            if (exchange_contacts(neighbours, neighbours[1].sockfd, n_neighbours, 1, table, FEDEX) == 0)
            {
                if (*n_neighbours > 1)
                {
                    //reordenar vizinhos internos
                    for (i = 3; i < *n_neighbours + 2; i++)
                    {
                        neighbours[i] = neighbours[i + 1];
                    }
                }
                //informar vizinhos internos do novo EXTERN
                inform_internal_newEXTERN(n_neighbours, neighbours, table, FEDEX);
                return 0;
            }
            printf("Neighbour didn't respond, trying next one...\n");
        }
    }
    printf("\nNobody is home :( . Awaiting for new connections\n");
    return -1;
}


/******************************************************************************
* 
*   Establishes a relation between this program and the person it's contacting.
*   Protocol goes like so:
*       -Send NEW and ADVERTISE protocols
*       -Wait for an answer
*           -If there's no answer in the designated wait time, connection fails
*           -If answer is received, function continues
*       -Read message received from contact and execute received protocols
*
*   Returns: (int)
*   0 if contacts were exchanged
*  -1 if process went abnormal
* 
******************************************************************************/
int exchange_contacts(neighbour *neighbours, int sockfd, int *n_neighbours, int index, expedition_table *table, object_search *FEDEX)
{
    int i;

    //Enviar mensagem de presen??a ao n?? externo com o nosso IP e TCP
    if (write_to_someone(neighbours[0].node.IP, neighbours[0].node.TCP, neighbours, "NEW", index, n_neighbours, table, FEDEX) == -1)
    {
        return -1;
    }

    for (i = 0; i < table->n_id; i++)
    {
        //Enviar mensagem de ADVERTISE de todas as entradas da tabela
        if (write_to_someone(table->id[i], "0", neighbours, "ADVERTISE", index, n_neighbours, table, FEDEX) == -1)
        {
            return -1;
        }
    }

    //Confirmar presen??a de um buffer para leitura
    if (wait_for_answer(sockfd, MAX_WAIT) == -1)
    {
        close_socket(n_neighbours, neighbours, index, table);
        return -1;
    }

    //Receber mensagem de presen??a do n?? externo com o IP e TCP do vizinho de recupera????o deles, e ADVERTISEs
    if (read_from_someone(neighbours, index, n_neighbours, table, FEDEX) == -1)
    {
        return -1;
    }
    return 0;
}

/*****************************************************************************
* 
*   In case someone drops connection with program.
*   Protocol goes like so:
*       -Close related socket
*       -Identify if in one of the following situations:
*           1 - Lost external neighbour and has a backup
*           2 - Lost external neighbour but backup is itself
*           3 - Same as 2 but has no neighbours
*       -Execute designated backup plan:
*           1 - Contact backup and hope he's alive, else leave network
*           2 - Promote internal neighbour to extern
*           3 - Just wait for new connections
*
*   Returns: (int)
*   0 if plan was successful
*  -1 if plan didn't went that smoothly
* 
******************************************************************************/
int backup_plan(int ready_index, int *n_neighbours, neighbour *placeholder, expedition_table *table, object_search *FEDEX)
{
    int fd_check, i;

    if (close_socket(n_neighbours, placeholder, ready_index, table) == -1)
    {
        return -1;
    }
    if(state == notreg)
    {
        return 0;
    }
    //promover vizinho de recupera????o a externo se recupera????o for diferente dele pr??prio
    if (ready_index == 1 && *n_neighbours >= 0 && ((strcmp(placeholder[2].node.IP, placeholder[0].node.IP) != 0) || (strcmp(placeholder[2].node.TCP, placeholder[0].node.TCP) != 0)))
    {
        placeholder[1] = placeholder[2];
        //Abrir conex??o TCP com o antigo n?? de recupera????o, agora externo
        placeholder[1].sockfd = TCP_client(placeholder[1].node.IP, placeholder[1].node.TCP, placeholder[1].node_info);
        fd_check = placeholder[1].sockfd;
        *n_neighbours += 1;

        //vizinho de recupera????o is AWOL
        if (fd_check == -1)
        {
            *n_neighbours -= 1;
            //atualizar para o estado leaving
            state = leaving;
            printf("\nYou have been disconnected due to abscence of contacts. Please try rejoining the network\n");
            return 0;
        }

        if (write_to_someone(placeholder[0].node.IP, placeholder[0].node.TCP, placeholder, "NEW", ready_index, n_neighbours, table, FEDEX) == -1)
        {
            if (promote_to_EXTERN(placeholder, n_neighbours, table, FEDEX) == -1)
            {
                placeholder[1] = placeholder[0];
                placeholder[2] = placeholder[0];
                state = lonereg;
            }
            return 0;
        }

        for (i = 0; i < table->n_id; i++)
        {
            //Enviar mensagem de ADVERTISE de todas as entradas da tabela
            if (write_to_someone(table->id[i], "0", placeholder, "ADVERTISE", 1, n_neighbours, table, FEDEX) == -1)
            {
                if (promote_to_EXTERN(placeholder, n_neighbours, table, FEDEX) == -1)
                {
                    placeholder[1] = placeholder[0];
                    placeholder[2] = placeholder[0];
                    state = lonereg;
                }
                return 0;
            }
        }

        //Confirmar presen??a de um buffer para leitura
        if (wait_for_answer(placeholder[1].sockfd, MAX_WAIT) == -1)
        {
            if (promote_to_EXTERN(placeholder, n_neighbours, table, FEDEX) == -1)
            {
                placeholder[1] = placeholder[0];
                placeholder[2] = placeholder[0];
                state = lonereg;
            }
            return 0;
        }

        if (read_from_someone(placeholder, 1, n_neighbours, table, FEDEX) == -1)
        {
            if (promote_to_EXTERN(placeholder, n_neighbours, table, FEDEX) == -1)
            {
                placeholder[1] = placeholder[0];
                placeholder[2] = placeholder[0];
                state = lonereg;
            }
            return 0;
        }
        //atualizar estado
        state = reg;
        //se recupera????o for promovido a externo, informar vizinhos internos
        inform_internal_newEXTERN(n_neighbours, placeholder, table, FEDEX);
    }
    //se o vizinho de recupera????o ?? o pr??rio
    else if (ready_index == 1 && *n_neighbours > 0 && ((strcmp(placeholder[2].node.IP, placeholder[0].node.IP) == 0) && (strcmp(placeholder[2].node.TCP, placeholder[0].node.TCP) == 0)))
    {
        if (promote_to_EXTERN(placeholder, n_neighbours, table, FEDEX) == -1)
        {
            placeholder[1] = placeholder[0];
            placeholder[2] = placeholder[0];
            state = lonereg;
        }
    }
    //se o vizinho de recupera????o ?? o pr??rio e n??o tem vizinhos internos
    else if (ready_index == 1 && *n_neighbours == 0 && ((strcmp(placeholder[2].node.IP, placeholder[0].node.IP) == 0) && (strcmp(placeholder[2].node.TCP, placeholder[0].node.TCP) == 0)))
    {
        //solu????o: esperar por resposta
        placeholder[1] = placeholder[0];
        state = lonereg;
    }
    return 0;
}

/*****************************************************************************
* 
*   Just informs every neighbour of your shiny new extern neighbour. 
*   Good for you!
*
*   Returns: (void)
*
******************************************************************************/
void inform_internal_newEXTERN(int *n_neighbours, neighbour *neighbours, expedition_table *table, object_search *FEDEX)
{
    int i;

    //se tiveres vizinhos
    if (*n_neighbours > 0)
    {
        //atualizar vizinhos com EXTERN
        for (i = 1; i < *n_neighbours + 2; i++)
        {
            if(i == 2)
                continue;

            if (write_to_someone(neighbours[1].node.IP, neighbours[1].node.TCP, neighbours, "EXTERN", i, n_neighbours, table, FEDEX) == -1)
            {
                i--;
                continue;
            }
        }
    }
    //se o teu vizinho interno for promovido a externo, mandar tamb??m mensagem
    /*
    if ((strcmp(neighbours[2].node.IP, neighbours[0].node.IP) == 0) && (strcmp(neighbours[2].node.TCP, neighbours[0].node.TCP) == 0))
    {
        write_to_someone(neighbours[1].node.IP, neighbours[1].node.TCP, neighbours, "EXTERN", 1, n_neighbours, table, FEDEX);
    }
    */
    return;
}