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

//Instruções para dar input corretamente após erro no terminal
void instructions()
{
    printf("To start this program, your command must be formatted like this:\n\n");
    printf("ndn <IP> <TCP> <regIP> <regUDP>\n\n");
    return;
}

/*
Validar IPv4 introduzido
Return:
    0 quando o IP for válido
    1 quando o contrário
*/
int validar_IPv4(char *IPv4)
{
    int flag = 0;
    char buf[sizeof(struct in_addr)];

    flag = inet_pton(AF_INET, IPv4, buf);
    if (flag != 1)
    {
        if (flag == 0)
            printf("Error found: IP inserted is not valid\n");
        if (flag == -1)
            printf("Error found: %s\n", strerror(errno));
        return 1;
    }
    return 0;
}

/*
Validar port number introduzido
Return:
    0 quando o port number for válido
    1 quando o contrário
*/
int validar_port(char *port)
{
    int port_number = atoi(port);

    if (port_number > 65536 || port_number < 1)
    {
        printf("Error found: port number inserted is not valid\n");
        return 1;
    }
    return 0;
}

//Validação do comando que inicia o programa
void validate_start(int argc, char *argv[])
{
    int flag = 0;

    if (argc < 3 || argc == 4)
    {
        printf("Error found: %s\n", strerror(EINVAL));
        instructions();
        exit(1);
    }
    if (argc > 5)
    {
        printf("Error found: %s\n", strerror(E2BIG));
        instructions();
        exit(1);
    }

    flag += validar_IPv4(argv[1]);
    flag += validar_port(argv[2]);

    if (argc > 3)
    {
        flag += validar_IPv4(argv[3]);
        flag += validar_port(argv[4]);
    }

    if (flag != 0)
    {
        instructions();
        exit(1);
    }
    return;
}

void user_interface()
{
    //variables
    char nodeid[BUF_SIZE];
    char command1[BUF_SIZE];
    char command2[BUF_SIZE];
    char bootIP[16];
    char bootTCP[6];
    int flag;
    char buffer[BUF_SIZE] ;

    //code
    if(fgets(buffer, BUF_SIZE, stdin) == NULL)
    {
        printf("Invalid command\n");
        return;
    }
    flag = sscanf(buffer, "%s %s %s %s %s", command1, command2, nodeid, bootIP, bootTCP);

    if (strcmp(command1, "join") == 0  && state == notreg)
    {
        if (flag == 3)
        {
            join_complicated(command2, nodeid);
        }

        else if (flag == 5 && validar_IPv4(bootIP) == 0 && validar_port(bootTCP) == 0)
        {
            //join_simple();
        }

        else
        {
            printf("Deu shit, go back");
        }
    }
    else if (strcmp(command1, "create") && flag == 2 && state != notreg)
    {
        //create_node(); //exemplo
    }
    else if (strcmp(command1, "get") && flag == 2 && state != notreg)
    {
    }
    else if (((strcmp(command1, "show") && flag == 2) || (((strcmp(command1, "st") || strcmp(command1, "sr") || strcmp(command1, "sc")) && (flag == 1)))) && state != notreg)
    {

        if (strcmp(command2, "topology") || strcmp(command1, "st"))
        {
        }

        else if (strcmp(command2, "routing") || strcmp(command1, "sr"))
        {
        }

        else if (strcmp(command2, "cache") || strcmp(command1, "sc"))
        {
        }
    }
    else if (strcmp(command1, "leave") && flag == 1 && state != notreg)
    {
    }
    else if (strcmp(command1, "exit") && flag == 1)
    {
    }
    //se não for encontrado qualquer comando da lista
    else
    {
        printf("Deu shit, go back");
    }

    return;
}