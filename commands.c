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

int join_complicated(char *netID, char *nodeID, int sock_server, char *nodeIP, char *nodeTCP, nodes topology[2], nodes* nodeslist)
{
    //variables
    //comando: REG (espaço) netID (espaço) IP (espaço) TCP
    int i = 0, flag = 0, left = 0;
    int max_buffer = 4 + strlen(netID) + 1 + strlen(nodeIP) + 1 + 5 + 1;
    int node_externo = 0;
    char bufferREG[max_buffer];
    char confirm_message[6];
    char *ptr, buffer[BUF_SIZE];

    struct sockaddr addr;
    socklen_t addrlen;

    ask_list(netID, sock_server, nodeslist);
    
    //Caso 1: Lista Vazia --> a topologia é o próprio nó 
    if(n_nodes == 0)
    {
        //Guardar vizinho externo
        strcpy(topology[0].IP,nodeIP);
        strcpy(topology[0].TCP,nodeTCP);

        //Guardar vizinho de recuperação
        strcpy(topology[1].IP,nodeIP);
        strcpy(topology[1].TCP,nodeTCP);
    }

    

    //Caso 2: Lista com só um nó
    if(n_nodes == 1)
    {
        //Criar processo TCP a ligar ao nó externo
        
        node_externo = TCP_client(nodeslist[0].IP, nodeslist[0].TCP, topology[0]);
    	if(node_externo != -1)
        {
            return -1;
        }

        //Enviar mensagem de presença ao nó externo com o nosso IP e TCP
        sprintf(buffer, "NEW %s %s\n", nodeIP, nodeTCP);
        left = strlen(buffer);
        ptr = buffer;

        while(left > 0)
        {
            flag = write(node_externo, ptr, left);
            //erro na escrita ou closed by peer
            if(flag == -1 || flag == 0)
            {
                return -1;
            }

            left -= flag;
            ptr += flag;
        }

        //Receber mensagem de presença do nó externo com o IP e TCP do vizinho de recuperação deles (neste caso a própria informação)
        

    }


    //Caso 3: Lista com mais que um nó
    if(n_nodes > 1)
    {
        //Escolher nó a que se liga
        //rand() % n_nodes;

        
    }
    
    
    
    
    
    
    //Mandar informação ao servidor a dizer que ligou (REG)
    sprintf(bufferREG, "REG %s %s %s", netID, nodeIP, nodeTCP); 
    if (sendto(sock_server, bufferREG , strlen(bufferREG) + 1, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    if(wait_for_answer(sock_server) == -1)
        return -1;

    if (recvfrom(sock_server, confirm_message, sizeof(confirm_message) + 1, 0, &addr, &addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    if(strcmp(confirm_message,"OKREG") != 0)
    {
        printf("No confirmation received from server. Returning...\n");
        return -1;
    }

    return 0;
}

int join_simple(char *netID, char *nodeID, char* bootIP, char* bootTCP)
{
    return 0;
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
    if (sendto(sock_server, bufferUNREG , strlen(bufferUNREG) + 1, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    if(wait_for_answer(sock_server) == -1)
        return -1;

    if (recvfrom(sock_server, confirm_message, sizeof(confirm_message) + 1, 0, &addr, &addrlen) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    if(strcmp(confirm_message,"OKUNREG") != 0)
    {
        printf("No confirmation received from server. Returning...\n");
        return -1;
    }

    //libertar info ?

    return 0;
}