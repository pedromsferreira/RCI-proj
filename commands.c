#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "validation.h"
#include "network.h"
#include "commands.h"

#define BUF_SIZE 1024

void join_complicated(char *netID, char *nodeID)
{
    ask_list(netID);
    //Escolher nó a que se liga
    //Criar processo TCP a ligar ao nó  
    //Mandar informação ao servidor a dizer que ligou (REG)
    return;
}

void join_simple(char *netID, char *nodeID, char* bootIP, char* bootTCP)
{
}