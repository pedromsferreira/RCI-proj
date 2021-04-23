#include <stdio.h>
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

/*
Validar comando inserido durante a sessão no stdin
Return:
    0 quando o comando for conhecido e bem executado
    -1 quando o contrário
*/
int user_interface(int sockfd, char* argv[], neighbour* neighbours, int* n_neighbours)
{
    char arguments[5][BUF_SIZE];
    int flag, error;
    char buffer[BUF_SIZE];

    //verificar o que foi escrito na consola
    if (fgets(buffer, BUF_SIZE, stdin) == NULL)
    {
        printf("Invalid command\n");
        return -1;
    }
    //distribuir em variáveis o que foi escrito na consola
    flag = sscanf(buffer, "%s %s %s %s %s", arguments[0], arguments[1], arguments[2], arguments[3], arguments[4]);

    //avaliar o comando dado pela consola segundo a cadeia de condições possíveis
    if (strcmp(arguments[0], "join") == 0 && state == notreg)
    {
        if (flag == 3)
        {
            error = join_complicated(arguments[1], arguments[2], sockfd, argv[1], argv[2], neighbours, n_neighbours);
            if (error == 0)
            {
                state = reg;
                return 0;
            }
                
            if (error == -1)
                return -1;
        }

        else if (flag == 5 && validar_IPv4(arguments[3]) == 0 && validar_port(arguments[4]) == 0)
        {
            //join_simple();
        }
    }
    else if (strcmp(arguments[0], "create") == 0 && flag == 2 && state != notreg)
    {
        //create_node(); //exemplo
    }
    else if (strcmp(arguments[0], "get") == 0 && flag == 2 && state != notreg)
    {
    }
    else if (((strcmp(arguments[0], "show") == 0 && flag == 2) || (((strcmp(arguments[0], "st") || strcmp(arguments[0], "sr") || strcmp(arguments[0], "sc")) && flag == 1))) && state != notreg)
    {

        if (strcmp(arguments[1], "topology") == 0 || strcmp(arguments[0], "st") == 0)
        {
        }

        else if (strcmp(arguments[1], "routing") == 0 || strcmp(arguments[0], "sr") == 0)
        {
        }

        else if (strcmp(arguments[1], "cache") == 0 || strcmp(arguments[0], "sc") == 0)
        {
        }
    }
    else if (strcmp(arguments[0], "leave") == 0 && flag == 1 && state != notreg)
    {
        //if(leave_server(netID,) == 0)
            state = notreg;
    }
    else if (strcmp(arguments[0], "exit") == 0 && flag == 1)
    {
    }

    //se não for encontrado qualquer comando da lista
    printf("Invalid command\n");
    return -1;
}