#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "defines.h"
#include "validation.h"
#include "network.h"
#include "commands.h"
#include "TCP.h"
#include "UDP.h"

int join_complicated(char *netID, char *nodeID, int sock_server, char *nodeIP, char *nodeTCP, neighbour *neighbours, int *n_neighbours /*, expedition_table* table*/)
{
    //variables
    //comando: REG (espaço) netID (espaço) IP (espaço) TCP
    int max_buffer = 4 + strlen(netID) + 1 + strlen(nodeIP) + 1 + 5 + 1;
    int node_externo = 0, flag = 0;
    int n_nodes, *shuffle, i, current = 0;
    char bufferREG[max_buffer];
    char confirm_message[BUF_SIZE];
    nodes nodeslist[MAX_NODES];

    struct sockaddr addr;
    socklen_t addrlen;

    ask_list(netID, sock_server, nodeslist, &n_nodes);

    //Criar fd exclusivo a listen
    neighbours[0].sockfd = TCP_server(nodeTCP, neighbours);
    if (neighbours[0].sockfd == -1)
    {
        return -1;
    }

    //Caso 1: Lista Vazia --> começar listen e entrar só no server
    if (n_nodes == 0)
    {
        printf("Looks like there's nobody registered. Joining alone...\n");
    }

    //Caso 2: Lista com só um nó
    if (n_nodes == 1)
    {
        //Criar processo TCP conexão a ligar ao nó externo
        node_externo = TCP_client(nodeslist[0].IP, nodeslist[0].TCP, neighbours[1].node_info);
        if (node_externo == -1)
        {
            //fechar listenfd
            close_listen(neighbours);
            return -1;
        }
        current = 0;
        printf("Connection established to someone! Waiting for information...\n");

        //info que falta guardar
        neighbours[1].sockfd = node_externo;
        *n_neighbours += 1;

        //tentar comunicar
        if (exchange_contacts(neighbours, node_externo, n_neighbours, 1) == -1)
        {
            //fechar listenfd
            close_listen(neighbours);
            return -1;
        }
    }

    //Caso 3: Lista com mais que um nó
    if (n_nodes > 1)
    {
        shuffle = (int *)malloc(n_nodes * sizeof(int));

        //Escolher nó a que se liga
        shuffle = random_neighbour(n_nodes, shuffle);

        for (i = 0; i < n_nodes; i++)
        {
            node_externo = TCP_client(nodeslist[shuffle[i]].IP, nodeslist[shuffle[i]].TCP, neighbours[1].node_info);
            //node_externo = TCP_client(nodeslist[0].IP, nodeslist[0].TCP, neighbours[1].node_info);
            if (node_externo == -1 && i == n_nodes - 1)
            {
                //fechar listenfd
                close_listen(neighbours);
                return -1;
            }
            //tentar comunicar
            else
            {
                //info que falta guardar
                neighbours[1].sockfd = node_externo;
                *n_neighbours += 1;

                flag = exchange_contacts(neighbours, node_externo, n_neighbours, 1);

                if (flag == -1 && i == n_nodes - 1)
                {
                    //fechar listenfd
                    close_listen(neighbours);
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

        printf("Information received! Registering in the nodes server...\n");
    }

    //Mandar informação ao servidor a dizer que ligou (REG)
    sprintf(bufferREG, "REG %s %s %s", netID, nodeIP, nodeTCP);
    if (sendto(sock_server, bufferREG, strlen(bufferREG) + 1, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        close_listen(neighbours);
        return -1;
    }

    if (wait_for_answer(sock_server, 2) == -1)
    {
        close_listen(neighbours);
        return -1;
    }

    if (recvfrom(sock_server, confirm_message, sizeof(confirm_message) + 1, 0, &addr, &addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        close_listen(neighbours);
        return -1;
    }
    if (strcmp(confirm_message, "OKREG") != 0)
    {
        printf("%s\n", confirm_message);
        close_listen(neighbours);
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
//join_complicated(char *netID, char *nodeID, int sock_server, char *nodeIP, char *nodeTCP, neighbour* neighbours, int* n_neighbours/*, expedition_table* table*/)
int join_simple(char *netID, char *nodeID, char *bootIP, char *bootTCP, int sock_server, char *nodeIP, char *nodeTCP, neighbour *neighbours, int *n_neighbours /*, expedition_table* table*/)
{
    int max_buffer = 4 + strlen(netID) + 1 + strlen(nodeIP) + 1 + 5 + 1;
    int node_externo = 0;
    char bufferREG[max_buffer];
    char confirm_message[BUF_SIZE];

    struct sockaddr addr;
    socklen_t addrlen;

    //Criar fd exclusivo a listen
    neighbours[0].sockfd = TCP_server(nodeTCP, neighbours);
    if (neighbours[0].sockfd == -1)
    {
        return -1;
    }

    //Criar processo TCP conexão a ligar ao nó externo
    node_externo = TCP_client(bootIP, bootTCP, neighbours[1].node_info);
    if (node_externo == -1)
    {
        //fechar listenfd
        close_listen(neighbours);
        return -1;
    }

    printf("Connection established to someone! Waiting for information...\n");

    //info que falta guardar
    neighbours[1].sockfd = node_externo;
    *n_neighbours += 1;

    //tentar comunicar
    if (exchange_contacts(neighbours, node_externo, n_neighbours, 1) == -1)
    {
        //fechar listenfd
        close_listen(neighbours);
        return -1;
    }

    //Dar update ao externo do nó na topologia
    strcpy(neighbours[1].node.IP, bootIP);
    strcpy(neighbours[1].node.TCP, bootTCP);
    printf("Information received! Registering in the nodes server...\n");

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

    if (recvfrom(sock_server, confirm_message, sizeof(confirm_message) + 1, 0, &addr, &addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    if (strcmp(confirm_message, "OKREG") != 0)
    {
        printf("%s\n", confirm_message);
        return -1;
    }

    printf("Join complete!\n");

    //update state
    state = reg;

    return 0;
}

void print_topology(neighbour *neighbours)
{
    printf("Extern ( ͡° ͜ʖ ͡°) :\n IP --> %s\n TCP --> %s\n", neighbours[1].node.IP, neighbours[1].node.TCP);
    printf("Recovery ( ͡° ͜ʖ ͡°) :\n IP --> %s\n TCP --> %s\n", neighbours[2].node.IP, neighbours[2].node.TCP);
}

void print_routing()
{

    return;
}

void print_cache()
{

    return;
}

int leave_server(char *netID, int sock_server, char *nodeIP, char *nodeTCP)
{
    //variables
    int max_buffer = 6 + strlen(netID) + 1 + strlen(nodeIP) + 1 + 5 + 1;
    char bufferUNREG[max_buffer];
    char confirm_message[8];

    struct sockaddr addr;
    socklen_t addrlen;

    //Mandar informação ao servidor a dizer que vai desligar (UNREG)
    sprintf(bufferUNREG, "UNREG %s %s %s", netID, nodeIP, nodeTCP);
    if (sendto(sock_server, bufferUNREG, strlen(bufferUNREG) + 1, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    if (wait_for_answer(sock_server, 2) == -1)
        return -1;

    if (recvfrom(sock_server, confirm_message, sizeof(confirm_message) + 1, 0, &addr, &addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    if (strcmp(confirm_message, "OKUNREG") != 0)
    {
        printf("No confirmation received from server. Returning...\n");
        return -1;
    }
    printf("Left with success!\n");
    return 0;
}

void execute_NEW(neighbour *neighbours, char *mail_sent, int n_neighbours, int ready_index)
{
    char arguments[3][BUF_SIZE];

    //ler IP e TCP do vizinho de recuperação
    sscanf(mail_sent, "%s %s %s\n", arguments[0], arguments[1], arguments[2]);

    //guardar info no lugar do vizinho externo se estiver no estado lonereg
    if (state == lonereg || ready_index == 1)
    {
        strcpy(neighbours[1].node.IP, arguments[1]);
        strcpy(neighbours[1].node.TCP, arguments[2]);
        return;
    }
    //Guardar Info no Lugar dos Vizinhos Internos
    else if (n_neighbours > 1)
    {
        strcpy(neighbours[n_neighbours + 2].node.IP, arguments[1]);
        strcpy(neighbours[n_neighbours + 2].node.TCP, arguments[2]);
        return;
    }
    return;
}

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

    //guardar info do vizinho de recuperação
    strcpy(neighbours[2].node.IP, arguments[1]);
    strcpy(neighbours[2].node.TCP, arguments[2]);

    return;
}

void execute_ADVERTISE(neighbour *neighbours, char *mail_sent)
{
    char arguments[2][BUF_SIZE];

    //ler ID
    sscanf(mail_sent, "%s %s\n", arguments[0], arguments[1]);

    //Guardar

    return;
}

void execute_WITHDRAW(neighbour *neighbours, char *mail_sent)
{
    char arguments[2][BUF_SIZE];

    //ler ID
    sscanf(mail_sent, "%s %s\n", arguments[0], arguments[1]);

    //Guardar

    return;
}

int close_all_sockets(int n_neighbours, neighbour *neighbours)
{
    int i;
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

int close_socket(int *n_neighbours, neighbour *neighbours, int chosen_index)
{
    int i;
    //Fecho do socket
    if (close(neighbours[chosen_index].sockfd) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        *n_neighbours -= 1;
        return -1;
    }
    neighbours[chosen_index].sockfd = -1;
    //Se o nó retirado for um interno, move a tabela 1 fila para cima
    if (chosen_index > 2 && *n_neighbours > 2)
    {
        for (i = 3; i < *n_neighbours + 2; i++)
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

int close_listen(neighbour *neighbours)
{
    if (close(neighbours[0].sockfd) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    freeaddrinfo(neighbours[0].node_info);
    return 0;
}