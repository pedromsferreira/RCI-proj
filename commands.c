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

int join_complicated(char *netID, char *nodeID, int sock_server)
{
    //variables


    ask_list(netID, sock_server);
    //random_picker(n_nodes);
    //Escolher nó a que se liga
    //Criar processo TCP a ligar ao nó  
    //Mandar informação ao servidor a dizer que ligou (REG)
    return 0;
}

int join_simple(char *netID, char *nodeID, char* bootIP, char* bootTCP)
{
    return 0;
}