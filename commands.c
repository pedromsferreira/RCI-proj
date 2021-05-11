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
#include "TCP.h"
#include "UDP.h"

/******************************************************************************
* Joins nodes server without having a specified node to connect
*
* Arguments:
*   netID - network name
*   nodeID - node identification in the network
*   sock_server - fd corresponding to UDP connection with server
*   nodeIP - node's IP
*   nodeTCP - node's port number
*   n_neighbours - number of your neighbours (should be 0)
*   
*
* Returns: (int)
*   0 if successfull
*  -1 if something went wrong
******************************************************************************/
int join_complicated(char *netID, char *nodeID, int sock_server, char *nodeIP, char *nodeTCP, neighbour *neighbours, int *n_neighbours, expedition_table *table, object_search *FEDEX)
{
    //variables
    //comando: REG (espaço) netID (espaço) IP (espaço) TCP
    int max_buffer = 4 + strlen(netID) + 1 + strlen(nodeIP) + 1 + 5 + 1;
    int node_externo = 0, flag = 0;
    int n_nodes, *shuffle, i, current = 0;
    char bufferREG[max_buffer];
    char confirm_message[BUF_SIZE];
    nodes nodeslist[MAX_NODES];

    /*struct sockaddr addr;
    socklen_t addrlen;*/

    memset(confirm_message, '\0', BUF_SIZE);

    ask_list(netID, sock_server, nodeslist, &n_nodes);

    //Criar fd exclusivo a listen
    neighbours[0].sockfd = TCP_server(nodeTCP, neighbours);
    if (neighbours[0].sockfd == -1)
    {
        return -1;
    }

    //atualizar tabela de expedição na posição 0
    insert_ID_in_table(table, neighbours[0].sockfd, nodeID);

    //Caso 1: Lista Vazia --> começar listen e entrar só no server
    if (n_nodes == 0)
    {
        printf("\nLooks like there's nobody registered. Joining alone...\n");
    }

    //Caso 2: Lista com só um nó
    if (n_nodes == 1)
    {
        //Criar processo TCP conexão a ligar ao nó externo
        node_externo = TCP_client(nodeslist[0].IP, nodeslist[0].TCP, neighbours[1].node_info);
        if (node_externo == -1)
        {
            //fechar listenfd
            close_listen(neighbours, table, FEDEX);
            return -1;
        }
        current = 0;

        //info que falta guardar
        neighbours[1].sockfd = node_externo;
        *n_neighbours += 1;

        //tentar comunicar
        if (exchange_contacts(neighbours, node_externo, n_neighbours, 1, table, NULL) == -1)
        {
            //fechar listenfd
            close_listen(neighbours, table, FEDEX);
            return -1;
        }
    }

    //Caso 3: Lista com mais que um nó
    if (n_nodes > 1)
    {
        shuffle = (int *)malloc(n_nodes * sizeof(int));
        //No caso de malloc inválido
        if(shuffle == NULL)
        {
            //fechar listenfd
            close_listen(neighbours, table, FEDEX);
            return -1;
        }

        //Escolher nó a que se liga
        shuffle = random_neighbour(n_nodes, shuffle);

        for (i = 0; i < n_nodes; i++)
        {
            node_externo = TCP_client(nodeslist[shuffle[i]].IP, nodeslist[shuffle[i]].TCP, neighbours[1].node_info);
            //node_externo = TCP_client(nodeslist[0].IP, nodeslist[0].TCP, neighbours[1].node_info);
            if (node_externo == -1 && i == n_nodes - 1)
            {
                //fechar listenfd
                close_listen(neighbours, table, FEDEX);
                free(shuffle);
                return -1;
            }
            //tentar comunicar
            else
            {
                //info que falta guardar
                neighbours[1].sockfd = node_externo;
                *n_neighbours += 1;

                flag = exchange_contacts(neighbours, node_externo, n_neighbours, 1, table, NULL);

                if (flag == -1 && i == n_nodes - 1)
                {
                    //fechar listenfd
                    close_listen(neighbours, table, FEDEX);
                    free(shuffle);
                    return -1;
                }
                if (flag == 0)
                {
                    break;
                }
                //printf("Connection established to someone! Waiting for information...\n");
            }
        }
        current = shuffle[i];
        free(shuffle);
    }

    if (n_nodes >= 1)
    {
        //Dar update ao externo do nó na topologia
        strcpy(neighbours[1].node.IP, nodeslist[current].IP);
        strcpy(neighbours[1].node.TCP, nodeslist[current].TCP);

    }

    //Mandar informação ao servidor a dizer que ligou (REG)
    sprintf(bufferREG, "REG %s %s %s", netID, nodeIP, nodeTCP);
    if (sendto(sock_server, bufferREG, strlen(bufferREG) + 1, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        close_listen(neighbours, table, FEDEX);
        return -1;
    }

    if (wait_for_answer(sock_server, 2) == -1)
    {
        close_listen(neighbours, table, FEDEX);
        return -1;
    }

    //addrlen=sizeof(addr);
    if (recvfrom(sock_server, confirm_message, sizeof(confirm_message) + 1, 0, NULL, NULL) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        close_listen(neighbours, table, FEDEX);
        return -1;
    }
    if (strcmp(confirm_message, "OKREG") != 0)
    {
        printf("Unexpected answer from nodes server:\n");
        printf("%s\n", confirm_message);
        close_listen(neighbours, table, FEDEX);
        return -1;
    }

    printf("Join complete!\n");

    //update state
    if (n_nodes == 0)
    {
        neighbours[1] = neighbours[0];
        neighbours[2] = neighbours[0];
        state = lonereg;
    }
    if (n_nodes >= 1)
    {
        state = reg;
    }
    return 0;
}

/******************************************************************************
* Joins nodes server with a specified node to connect
*
* Arguments:
*   netID - network name
*   nodeID - this node identification in the network
*   sock_server - fd corresponding to UDP connection with server
*   bootIP - target node's IP
*   bootTCP - target port number
*   nodeIP - this node's IP
*   nodeTCP - this node's port number
*   n_neighbours - number of your neighbours (should be 0)
*   
*
* Returns: (int)
*   0 if successfull
*  -1 if something went wrong
******************************************************************************/
int join_simple(char *netID, char *nodeID, char *bootIP, char *bootTCP, int sock_server, char *nodeIP, char *nodeTCP, neighbour *neighbours, int *n_neighbours, expedition_table *table, object_search *FEDEX)
{
    int max_buffer = 4 + strlen(netID) + 1 + strlen(nodeIP) + 1 + 5 + 1;
    int node_externo = 0;
    char bufferREG[max_buffer];
    char confirm_message[BUF_SIZE];

    //struct sockaddr addr;
    //socklen_t addrlen;

    memset(confirm_message, '\0', BUF_SIZE);

    //Criar fd exclusivo a listen
    neighbours[0].sockfd = TCP_server(nodeTCP, neighbours);
    if (neighbours[0].sockfd == -1)
    {
        return -1;
    }

    //atualizar tabela de expedição na posição 0
    insert_ID_in_table(table, neighbours[0].sockfd, nodeID);

    //Criar processo TCP conexão a ligar ao nó externo
    node_externo = TCP_client(bootIP, bootTCP, neighbours[1].node_info);
    if (node_externo == -1)
    {
        //fechar listenfd
        close_listen(neighbours, table, FEDEX);
        return -1;
    }

    //info que falta guardar
    neighbours[1].sockfd = node_externo;
    *n_neighbours += 1;

    //tentar comunicar
    if (exchange_contacts(neighbours, node_externo, n_neighbours, 1, table, NULL) == -1)
    {
        //fechar listenfd
        close_listen(neighbours, table, FEDEX);
        return -1;
    }

    //Dar update ao externo do nó na topologia
    strcpy(neighbours[1].node.IP, bootIP);
    strcpy(neighbours[1].node.TCP, bootTCP);

    //Envio da mensagem de REG
    sprintf(bufferREG, "REG %s %s %s", netID, nodeIP, nodeTCP);
    if (sendto(sock_server, bufferREG, strlen(bufferREG) + 1, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    if (wait_for_answer(sock_server, 2) == -1)
    {
        return -1;
    }

    //addrlen=sizeof(addr);
    if (recvfrom(sock_server, confirm_message, sizeof(confirm_message) + 1, 0, NULL, NULL) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    if (strcmp(confirm_message, "OKREG") != 0)
    {
        printf("Unexpected answer from nodes server:\n");
        printf("%s\n", confirm_message);
        return -1;
    }

    printf("Join complete!\n");

    //update state
    state = reg;

    return 0;
}

/******************************************************************************
* Creates an object and stores it in the internal memory (see struct object_search's
* declaration for reference)
*
* Arguments:
*   ID - this node's ID
*   subname - user specified name for object
*
* Returns: (void)
*
******************************************************************************/
void create_subname(char *ID, char *subname, object_search *FEDEX)
{
    char buffer[BUF_SIZE];
    int i;

    //impor formato do nome do objeto
    sprintf(buffer, "%s.%s", ID, subname);

    //copiar objeto para a estrutura e incrementar número de objetos
    if (FEDEX->n_objects < MAX_OBJECTS)
    {
        for (i = 0; i < MAX_OBJECTS; i++)
        {
            if (strcmp(FEDEX->objects[i], buffer) == 0)
            {
                printf("\nWe already had one of those, but thanks for reminding us.\n");
                return;
            }
        }
        strcpy(FEDEX->objects[FEDEX->n_objects], buffer);
        FEDEX->n_objects += 1;
    }
    else if (FEDEX->n_objects == MAX_OBJECTS)
    {
        printf("\nExceeded capacity, removing the oldest object in record...");
        for (i = 0; i < MAX_OBJECTS - 1; i++)
        {
            strcpy(FEDEX->objects[i], FEDEX->objects[i + 1]);
        }
        strcpy(FEDEX->objects[MAX_OBJECTS - 1], buffer);
        printf("\nDone.\n");
    }
    return;
}


/******************************************************************************
* Clean objects stored in internal memory depending on option
*
* Arguments:
*   ID - this node's ID
*   subname - user specified name for object
*   option:
*       0 - clear all
*       1 - clear only subname
*
* Returns: (void)
*
******************************************************************************/
void clean_objects(char *ID, char *subname, object_search *FEDEX, int option)
{
    char buffer[BUF_SIZE];
    int i;

    //clear all
    if (option == 0)
    {
        for (i = 0; i < MAX_OBJECTS; i++)
        {
            memset(FEDEX->objects[i], '\0', BUF_SIZE);
        }
        FEDEX->n_objects = 0;
    }
    //clear subname
    else if (option == 1)
    {
        //impor formato do nome do objeto
        sprintf(buffer, "%s.%s", ID, subname);

        for (i = 0; i < MAX_OBJECTS; i++)
        {
            if (strcmp(buffer, FEDEX->objects[i]) == 0)
            {
                memset(FEDEX->objects[i], '\0', BUF_SIZE);
                FEDEX->n_objects -= 1;
                return;
            }
        }
    }
    return;
}

/******************************************************************************
* Sends an INTEREST message to a node in same direction of target object requested
*
* Arguments:
*   name - target object
*   n_neighbours - number of known neighbours
*
* Returns: (void)
*
******************************************************************************/
void start_search_for_object(char *name, object_search *FEDEX, expedition_table *table, neighbour *neighbours, int *n_neighbours)
{
    char ID[BUF_SIZE], subname[BUF_SIZE];
    int destination_table, destination;

    if (strcmp(FEDEX->cache_objects[0], name) == 0 || strcmp(FEDEX->cache_objects[1], name) == 0)
    {
        /*Mandar DATA*/
        printf("\nAlready in cache, but OK chief.\n");
        return;
    }

    //verificar se existe um id na rede como o especificado
    if (separate_ID_subname(name, ID, subname, table) == -1)
    {
        printf("\nID unknown in the network. Be sure the object you are searching is in this format:\n");
        printf("\n      <id>.<subname>      \n");
        return;
    }

    //descobrir qual o socket para onde mandar pela tabela de expedição
    destination_table = find_ID_index_in_expedition_table(ID, table);
    destination = find_ID_index_in_struct_neighbours(neighbours, table, n_neighbours, destination_table);

    //enviar mensagem de INTEREST para o socket correspondente na tabela de expedição
    if (write_to_someone(name, "0", neighbours, "INTEREST", destination, n_neighbours, table, FEDEX) == -1)
    {
        printf("\nIntermediate node disconnected while sending message. Please try again.\n");
        return;
    }

    //atualizar em FEDEX que estamos neste momento à espera de um objeto
    strcpy(FEDEX->ID_return[FEDEX->n_return], table->id[0]);
    strcpy(FEDEX->object_return[FEDEX->n_return], name);
    FEDEX->timer[FEDEX->n_return] = time(NULL);
    FEDEX->n_return++;

    return;
}

/******************************************************************************
* Shows in console current external and recovery neighbour
*
* Returns: (void)
******************************************************************************/
void print_topology(neighbour *neighbours)
{
    printf("Extern ( ͡° ͜ʖ ͡°) :\n IP --> %s\n TCP --> %s\n", neighbours[1].node.IP, neighbours[1].node.TCP);
    printf("Recovery ( ͡° ͜ʖ ͡°) :\n IP --> %s\n TCP --> %s\n", neighbours[2].node.IP, neighbours[2].node.TCP);
    return;
}

/******************************************************************************
* Shows in console every entry in expedition table
*
* Returns: (void)
******************************************************************************/
void print_routing(expedition_table table)
{
    for (int i = 0; i < table.n_id; i++)
    {
        if (i == 0)
        {
            printf("( ͡° ͜ʖ ͡°) : ID --> %s // fd --> -\n", table.id[i]);
            continue;
        }

        printf("( ͡° ͜ʖ ͡°) : ID --> %s // fd --> %d\n", table.id[i], table.sockfd[i]);
    }
    return;
}

/******************************************************************************
* Shows in console every entry in cache
*
* Returns: (void)
******************************************************************************/
void print_cache(object_search *FEDEX)
{
    printf("\n( ͡° ͜ʖ ͡°) : Linha 1 --> %s\n", FEDEX->cache_objects[0]);
    printf("( ͡° ͜ʖ ͡°) : Linha 2 --> %s\n", FEDEX->cache_objects[1]);
    return;
}

/******************************************************************************
* Shows in console every entry in internal memory
*
* Returns: (void)
******************************************************************************/
void print_objects(object_search *FEDEX)
{
    for (int i = 0; i < FEDEX->n_objects; i++)
    {
        printf("\n( ͡° ͜ʖ ͡°) : Object %d --> %s", i + 1, FEDEX->objects[i]);
    }
    printf("\n");
    return;
}

/******************************************************************************
* Leaves nodes server (unregisters this node from current network)
*
* Arguments:
*   netID - network name
*   sock_server - fd corresponding to UDP connection with server
*   nodeIP - node's IP
*   nodeTCP - node's port number
*   
*
* Returns: (int)
*   0 - looks good
*  -1 - something wrong  
*
******************************************************************************/
int leave_server(char *netID, int sock_server, char *nodeIP, char *nodeTCP)
{
    //variables
    int max_buffer = 6 + strlen(netID) + 1 + strlen(nodeIP) + 1 + 5 + 1;
    char bufferUNREG[max_buffer];
    char confirm_message[BUF_SIZE];

    /*struct sockaddr addr;
    socklen_t addrlen;*/

    memset(confirm_message, '\0', BUF_SIZE);

    //Mandar informação ao servidor a dizer que vai dar leave da rede (UNREG)
    sprintf(bufferUNREG, "UNREG %s %s %s", netID, nodeIP, nodeTCP);
    if (sendto(sock_server, bufferUNREG, strlen(bufferUNREG) + 1, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    if (wait_for_answer(sock_server, 2) == -1)
        return -1;

    //addrlen=sizeof(addr);
    if (recvfrom(sock_server, confirm_message, sizeof(confirm_message) + 1, 0, NULL, NULL) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    if (strcmp(confirm_message, "OKUNREG") != 0)
    {
        printf("Unexpected answer from nodes server:\n");
        printf("%s\n", confirm_message);
        return -1;
    }
    printf("Left with success!\n");
    return 0;
}

/******************************************************************************
*   Protocol for leaving a network
*   Starts by leaving the nodes server, then proceeds to wipe all memory stored
*
* Arguments:
*   netID - network name
*   sock_server - fd corresponding to UDP connection with server
*   nodeIP - node's IP
*   nodeTCP - node's port number
*   
*
* Returns: (int)
*   0 - looks good
*  -1 - something wrong  
*
******************************************************************************/
int leave_protocol(char *netID, int sockfd, char **argv, int *n_neighbours, neighbour *neighbours, expedition_table *table, object_search *FEDEX)
{
    //de-register
    if (leave_server(netID, sockfd, argv[1], argv[2]) == -1)
    {
        printf("Something went wrong. Please try again.\n");
        return -1;
    }

    //fechar todos os fds e addrinfo
    if (*n_neighbours == 0)
    {
        close_listen(neighbours, table, FEDEX);
    }
    else if (*n_neighbours > 0)
    {
        close_all_sockets(*n_neighbours, neighbours, table, FEDEX);
    }
    return 0;
}

/******************************************************************************
* Runs when node receives command "NEW"
* Checks number of neighbours
*   - if it was alone or was sent by external node, store info and send "EXTERN" and "ADVERTISE" of his own network
*   - if it was internal that sent message, store info and send "EXTERN" and "ADVERTISE" of his own network
******************************************************************************/
void execute_NEW(neighbour *neighbours, char *mail_sent, int *n_neighbours, int ready_index, expedition_table *table)
{
    int i = 0;
    char arguments[3][BUF_SIZE];

    //ler IP e TCP do vizinho de recuperação
    sscanf(mail_sent, "%s %s %s\n", arguments[0], arguments[1], arguments[2]);

    //guardar info no lugar do vizinho externo se estiver no estado lonereg
    if (state == lonereg || ready_index == 1)
    {
        strcpy(neighbours[1].node.IP, arguments[1]);
        strcpy(neighbours[1].node.TCP, arguments[2]);
        //após receber mensagem de NEW, enviar EXTERN e ADVERTISE
        write_to_someone(neighbours[1].node.IP, neighbours[1].node.TCP, neighbours, "EXTERN", ready_index, n_neighbours, table, NULL);
        for (i = 0; i < table->n_id; i++)
        {
            //Enviar mensagem de ADVERTISE de todas as entradas da tabela
            write_to_someone(table->id[i], "0", neighbours, "ADVERTISE", ready_index, n_neighbours, table, NULL);
        }
        return;
    }
    //Guardar Info no Lugar dos Vizinhos Internos
    else if (*n_neighbours > 1)
    {
        strcpy(neighbours[*n_neighbours + 1].node.IP, arguments[1]);
        strcpy(neighbours[*n_neighbours + 1].node.TCP, arguments[2]);
        //após receber mensagem de NEW, enviar EXTERN e ADVERTISE
        write_to_someone(neighbours[1].node.IP, neighbours[1].node.TCP, neighbours, "EXTERN", ready_index, n_neighbours, table, NULL);
        for (i = 0; i < table->n_id; i++)
        {
            //Enviar mensagem de ADVERTISE de todas as entradas da tabela
            write_to_someone(table->id[i], "0", neighbours, "ADVERTISE", ready_index, n_neighbours, table, NULL);
        }
        return;
    }
    return;
}

/******************************************************************************
* Runs when node receives command "EXTERN"
* Updates recovery neighbour
******************************************************************************/
void execute_EXTERN(neighbour *neighbours, char *mail_sent, int ready_index)
{
    char arguments[3][BUF_SIZE];

    //ignorar mensagens de outros nós neste caso
    if (ready_index != 1)
    {
        return;
    }

    //ler IP e TCP do vizinho de recuperação
    sscanf(mail_sent, "%s %s %s\n", arguments[0], arguments[1], arguments[2]);

    //for debugging only
    //printf("\nRECOVERY updated to %s\n", arguments[2]);

    //guardar info do vizinho de recuperação
    strcpy(neighbours[2].node.IP, arguments[1]);
    strcpy(neighbours[2].node.TCP, arguments[2]);

    return;
}

/******************************************************************************
* Runs when node receives command "ADVERTISE"
* Inserts ID in table
* Sends "ADVERTISE ID" message to all nodes he's connected, except the node who sent the message
******************************************************************************/
void execute_ADVERTISE(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours)
{
    char arguments[2][BUF_SIZE];

    //Ler ID
    sscanf(mail_sent, "%s %s\n", arguments[0], arguments[1]);

    //Guardar na tabela de expedição current socket e ID recebido
    insert_ID_in_table(table, neighbours[ready_index].sockfd, arguments[1]);

    //Do not message neighbours if id already known
    /*if (flag == -1)
        return;*/

    if (*n_neighbours > 0)
    {
        //Contactar todos os vizinhos exceto o que te mandou a mensagem
        for (int i = 1; i < *n_neighbours + 2; i++)
        {
            if (i == 2 || i == ready_index)
                continue;

            if (write_to_someone(table->id[table->n_id - 1], "0", neighbours, "ADVERTISE", i, n_neighbours, table, NULL) == -1)
            {
                i--;
            }
        }
    }
    return;
}

/******************************************************************************
* Runs when node receives command "WITHDRAW"
* Removes ID from table
* Sends "WITHDRAW ID" message to all nodes he's connected, except the node who sent the message
******************************************************************************/
void execute_WITHDRAW(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours)
{
    char arguments[2][BUF_SIZE];

    //Ler ID
    sscanf(mail_sent, "%s %s\n", arguments[0], arguments[1]);

    //Remover da tabela de expedição o ID
    remove_ID_from_table(table, arguments[1]);

    if (*n_neighbours > 0)
    {
        //Contactar todos os vizinhos exceto o que te mandou a mensagem
        for (int i = 1; i < *n_neighbours + 2; i++)
        {
            if (i == 2 || i == ready_index)
                continue;

            if (write_to_someone(arguments[1], "0", neighbours, "WITHDRAW", i, n_neighbours, table, NULL) == -1)
            {
                close_socket(n_neighbours, neighbours, i, table);
                i--;
            }
        }
    }

    return;
}

/******************************************************************************
* Runs when node receives command "INTEREST"
* Checks cache to see if file is there, if is, send "DATA" towards source node
* If not destination node, sends message towards destination node
* If destination node, checks if exists in node "internal memory"
*   - if exists, sends "DATA" towards source node
*   - if doesn't exists, sends "NODATA" towards source node
******************************************************************************/
void execute_INTEREST(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours, object_search *FEDEX)
{
    char arguments[2][BUF_SIZE];
    char subname[BUF_SIZE];
    char ID[BUF_SIZE];
    int flag, i, j;

    //Ler mensagem recebida
    sscanf(mail_sent, "%s %s\n", arguments[0], arguments[1]);

    //Separar ID do subnome
    flag = separate_ID_subname(arguments[1], ID, subname, table);

    //Caso não tenha encontrado um ID válido, ignorar mensagem
    if (flag == -1)
    {
        return;
    }

    //Verifica a cache
    if (strcmp(FEDEX->cache_objects[0], arguments[1]) == 0 || strcmp(FEDEX->cache_objects[1], arguments[1]) == 0)
    {
        /*Mandar DATA*/
        write_to_someone(arguments[1], "0", neighbours, "DATA", ready_index, n_neighbours, table, FEDEX);
        return;
    }

    //Caso tenha chegado ao nó de destino
    if (strcmp(table->id[0], ID) == 0)
    {
        //Vai procurar nos objetos do nó
        for (i = 0; i < FEDEX->n_objects; i++)
        {
            //Se o objeto for encontrado
            if (strcmp(FEDEX->objects[i], arguments[1]) == 0)
            {
                /*Encontrou objeto e vai mandar DATA*/
                write_to_someone(arguments[1], "0", neighbours, "DATA", ready_index, n_neighbours, table, FEDEX);

                //Guardar na cache
                store_in_cache(FEDEX, arguments[1]);
                return;
            }
        }
        /*Não encontrou objeto e vai mandar NODATA*/
        write_to_someone(arguments[1], "0", neighbours, "NODATA", ready_index, n_neighbours, table, FEDEX);
        return;
    }

    //Caso não seja o nó de destino
    //Envio de mensagens de INTEREST em direção do nó que tem informação

    //Analisar que socket tem de utilizar para mandar mensagem de Interest
    for (i = 0; i < table->n_id; i++)
    {
        if (strcmp(table->id[i], ID) == 0)
        {
            //Procurar o índice de neighbours
            j = find_ID_index_in_struct_neighbours(neighbours, table, n_neighbours, i);

            write_to_someone(arguments[1], "0", neighbours, "INTEREST", j, n_neighbours, table, FEDEX);
        }
    }

    for (i = 0; i < table->n_id; i++)
    {
        if (table->sockfd[i] == neighbours[ready_index].sockfd)
        {
            break;
        }
    }
    //Guardar ID de retorno e objeto correspondente
    strcpy(FEDEX->ID_return[FEDEX->n_return], table->id[i]);
    strcpy(FEDEX->object_return[FEDEX->n_return], arguments[1]);
    FEDEX->timer[FEDEX->n_return] = time(NULL);
    FEDEX->n_return++;

    return;
}

/******************************************************************************
* Runs when node receives command "DATA"
* Stores in cache the file
* If not source node, sends message towards source node
* If source node, prints file to user
******************************************************************************/
void execute_DATA(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours, object_search *FEDEX)
{
    char arguments[2][BUF_SIZE];
    char subname[BUF_SIZE];
    char ID[BUF_SIZE];
    int i, j, k;

    //Ler mensagem recebida
    sscanf(mail_sent, "%s %s\n", arguments[0], arguments[1]);

    //Separar ID do subnome
    separate_ID_subname(arguments[1], ID, subname, table);

    //Guardar na cache
    store_in_cache(FEDEX, arguments[1]);

    //Se encontrar o nó que fez o request
    for (i = 0; i < FEDEX->n_return; i++)
    {
        //Se corresponder ao nó de origem
        if (strcmp(table->id[0], FEDEX->ID_return[i]) == 0 && strcmp(arguments[1], FEDEX->object_return[i]) == 0)
        {
            printf("\nObject found, here is your file :) --> %s\n", subname);
            update_line_return_FEDEX(FEDEX, i);

            return;
        }
    }

    /*Envio de mensagens DATA em direção do nó de origem*/
    //Analisar que socket tem de utilizar para mandar mensagem de Interest

    //Procura pelo índice do sockfd na expedition table
    for (i = 0; i < table->n_id; i++)
    {
        for (j = 0; j < FEDEX->n_return; j++)
        {
            //Determinar o índice do sockfd na expedition table
            if (strcmp(table->id[i], FEDEX->ID_return[j]) == 0 && strcmp(arguments[1], FEDEX->object_return[j]) == 0)
            {
                k = find_ID_index_in_struct_neighbours(neighbours, table, n_neighbours, i);

                /*Mandar DATA*/
                write_to_someone(arguments[1], "0", neighbours, "DATA", k, n_neighbours, table, FEDEX);

                //Limpar linha utilizada
                update_line_return_FEDEX(FEDEX, j);

                return;
            }
        }
    }
    return;
}

/******************************************************************************
* Runs when node receives command "NODATA"
* If not source node, sends message towards source node
* If source node, prints "Couldn't fetch file" to user
******************************************************************************/
void execute_NODATA(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours, object_search *FEDEX)
{
    char arguments[2][BUF_SIZE];
    char subname[BUF_SIZE];
    char ID[BUF_SIZE];
    int i, j, k;

    //Ler mensagem recebida
    sscanf(mail_sent, "%s %s\n", arguments[0], arguments[1]);

    //Separar ID do subnome
    separate_ID_subname(arguments[1], ID, subname, table);

    //Se encontrar o nó que fez o request
    for (i = 0; i < FEDEX->n_return; i++)
    {
        //Se corresponder ao nó de origem
        if (strcmp(table->id[0], FEDEX->ID_return[i]) == 0 && strcmp(arguments[1], FEDEX->object_return[i]) == 0)
        {
            printf("\nObject not found, might have been lost due to bad shipping companies :(\n");
            update_line_return_FEDEX(FEDEX, i);

            return;
        }
    }

    /*Envio de mensagens NODATA em direção do nó de origem*/
    //Procura pelo índice do sockfd na expedition table
    for (i = 0; i < table->n_id; i++)
    {
        for (j = 0; j < FEDEX->n_return; j++)
        {
            //Determinar o índice do sockfd na expedition table
            if (strcmp(table->id[i], FEDEX->ID_return[j]) == 0 && strcmp(arguments[1], FEDEX->object_return[j]) == 0)
            {
                k = find_ID_index_in_struct_neighbours(neighbours, table, n_neighbours, i);

                /*Mandar NODATA*/
                write_to_someone(arguments[1], "0", neighbours, "NODATA", k, n_neighbours, table, FEDEX);

                //Limpar linha utilizada
                update_line_return_FEDEX(FEDEX, j);

                return;
            }
        }
    }
    return;
}

/******************************************************************************
* Closes all listen and connection sockets
* Resets structs and frees addrinfo
*
* Returns: (int)
*   0 if successful
*   -1 if ERROR
******************************************************************************/
int close_all_sockets(int n_neighbours, neighbour *neighbours, expedition_table *table, object_search *FEDEX)
{
    int i;

    //Reset à tabela de expedição e de objetos
    reset_table(table);
    reset_objects(FEDEX);

    for (i = 0; i <= n_neighbours + 1; i++)
    {
        //o 2 é o vizinho de recuperação
        if (i == 2)
        {
            continue;
        }
        else if (neighbours[i].sockfd != 0 && neighbours[i].sockfd != -1)
        {
            if (close(neighbours[i].sockfd) == -1)
                return -1;
            neighbours[i].sockfd = -1;
        }
    }
    freeaddrinfo(neighbours[0].node_info);
    return 0;
}

/******************************************************************************
* Closes specified connection socket stored in neighbours struct
* Updates structs and topology accordingly
*
* Returns: (int)
*   0 if successful
*   -1 if ERROR
******************************************************************************/
int close_socket(int *n_neighbours, neighbour *neighbours, int chosen_index, expedition_table *table)
{
    int i;

    //Mandar mensagem de withdraw a todos os vizinhos
    if (*n_neighbours > 0)
    {
        //Contactar todos os vizinhos exceto o que te mandou a mensagem
        for (int i = 1; i < *n_neighbours + 2; i++)
        {
            if (i == 2 || i == chosen_index)
                continue;
            //Percorrer a tabela e identificar as linhas com sockfd iguais ao que se vai remover
            for (int j = 1; j < table->n_id; j++)
            {
                if (table->sockfd[j] == neighbours[chosen_index].sockfd)
                {
                    if (write_to_someone(table->id[j], "0", neighbours, "WITHDRAW", i, n_neighbours, table, NULL) == -1)
                    {
                        //em caso de desconexão
                        i--;
                        break;
                    }
                }
            }
        }
    }

    //Remover da tabela de expedição o id correspondente ao socket
    remove_socket_from_table(table, neighbours[chosen_index].sockfd);

    //Fecho do socket
    if (neighbours[chosen_index].sockfd != -1 && neighbours[chosen_index].sockfd != 0)
    {
        if (close(neighbours[chosen_index].sockfd) == -1)
        {
            printf("Error: %s\n", strerror(errno));
        }
    }

    neighbours[chosen_index].sockfd = -1;
    //Se o nó retirado for um interno, move a tabela 1 fila para cima
    if (chosen_index > 2 && *n_neighbours > 2)
    {
        for (i = chosen_index; i < *n_neighbours + 2; i++)
        {
            neighbours[i] = neighbours[i + 1];
        }
    }
    *n_neighbours -= 1;
    //Se ficar vazio na lista, torna-se o seu próprio externo e vizinho de recuperação
    if (*n_neighbours == 0)
    {
        neighbours[1] = neighbours[0];
        state = lonereg;
        return 0;
    }

    return 0;
}

/******************************************************************************
* Closes socket dedicated to listen
* Resets structs and frees addrinfo
******************************************************************************/
void close_listen(neighbour *neighbours, expedition_table *table, object_search *FEDEX)
{

    if (neighbours[0].sockfd != -1 && neighbours[0].sockfd != 0)
    {
        if (close(neighbours[0].sockfd) == -1)
        {
            printf("Error: %s\n", strerror(errno));
        }
    }

    remove_socket_from_table(table, neighbours[0].sockfd);

    reset_objects(FEDEX);

    neighbours[0].sockfd = -1;

    freeaddrinfo(neighbours[0].node_info);
    return;
}

/******************************************************************************
* Sets table struct to NULL pointer in strings and -1 in ints (sockets)
******************************************************************************/
void reset_table(expedition_table *table)
{
    //Tabela de ids
    for (int i = 0; i < MAX_NEIGHBOURS; i++)
    {
        memset(table->id[i], '\0', BUF_SIZE);
        table->sockfd[i] = -1;
    }
    table->n_id = 0;
    return;
}

/******************************************************************************
* Sets object_search struct to NULL pointer in strings
******************************************************************************/
void reset_objects(object_search *FEDEX)
{
    for (int i = 0; i < MAX_OBJECTS; i++)
    {
        memset(FEDEX->objects[i], '\0', BUF_SIZE);
        memset(FEDEX->ID_return[i], '\0', BUF_SIZE);
        memset(FEDEX->object_return[i], '\0', BUF_SIZE);
        FEDEX->timer[i] = 0;
        
    }
    FEDEX->n_objects = 0;
    FEDEX->n_return = 0;
    memset(FEDEX->cache_objects[0], '\0', BUF_SIZE);
    memset(FEDEX->cache_objects[1], '\0', BUF_SIZE);
    return;
}

/******************************************************************************
* Erases line of return ID and object
* Concatenates table
******************************************************************************/
void update_line_return_FEDEX(object_search *FEDEX, int index)
{
    memset(FEDEX->ID_return[index], '\0', BUF_SIZE);
    memset(FEDEX->object_return[index], '\0', BUF_SIZE);
    FEDEX->timer[index] = 0;

    for (int i = index; i < FEDEX->n_return - 1; i++)
    {
        strcpy(FEDEX->ID_return[i], FEDEX->ID_return[i + 1]);
        strcpy(FEDEX->object_return[i], FEDEX->object_return[i + 1]);
        FEDEX->timer[i] = FEDEX->timer[i + 1];
    }
    FEDEX->n_return--;
    return;
}

/******************************************************************************
* Store in cache ID_subname using LRU (Least Recently Used) policy
* First line is always most recent
* Last line always least recent
******************************************************************************/
void store_in_cache(object_search *FEDEX, char *ID_subname)
{
    int flag = 0, i;

    for (i = 1; i >= 0; i--)
    {
        //Se a fila da cache estiver vazia
        if (FEDEX->cache_objects[i] == 0)
        {
            //Copia id.subname
            strcpy(FEDEX->cache_objects[i], ID_subname);
            flag = 1;
            break;
        }
    }

    if (flag == 0)
    {
        //Se a cache estiver cheia, puxar a informação uma linha para baixo e introduzir nova informação na primeira linha
        strcpy(FEDEX->cache_objects[1], FEDEX->cache_objects[0]);
        strcpy(FEDEX->cache_objects[0], ID_subname);
    }
    return;
}
/******************************************************************************
* Checks clock when called to see if a search for an object has expired
******************************************************************************/
void check_clock(object_search *FEDEX)
{
    int i;
    time_t stoptime;

    for (i = 0; i < FEDEX->n_return; i++)
    {
        stoptime = time(NULL) - FEDEX->timer[i];
        if (stoptime > WAIT_TIME)
        {
            printf("\nObject %s seems to be stuck. Maybe try asking again?\n", FEDEX->object_return[i]);
            update_line_return_FEDEX(FEDEX, i);
            printf("\nndn> ");
            fflush(stdout);
        }
        //for debugging only
        //printf("\nTimer check: %ld\n", stoptime);
    }
    
    return;
}
