#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "defines.h"
#include "network.h"
#include "commands.h"
#include "validation.h"

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
    -1 quando o contrário
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
        return -1;
    }
    return 0;
}

/*
Validar port number introduzido
Return:
    0 quando o port number for válido
    -1 quando o contrário
*/
int validar_port(char *port)
{
    int port_number = atoi(port);

    if (port_number > 65536 || port_number < 1)
    {
        printf("Error found: port number inserted is not valid\n");
        return -1;
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
    1 quando o programa for encerrar
    0 quando o comando for conhecido e bem executado
    -1 quando o contrário
*/
int user_interface(int sockfd, char* argv[], neighbour* neighbours, int* n_neighbours, char* netID/*, expedition_table* table*/)
{
    char arguments[5][BUF_SIZE];
    int flag, error, i;
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
            error = join_complicated(arguments[1], arguments[2], sockfd, argv[1], argv[2], neighbours, n_neighbours/*, table*/);
            if (error == 0)
            {
                strcpy(netID, arguments[1]);
                return 0;
            }
                
            if (error == -1)
            {
                printf("Something went wrong. Please try again.\n");
                return -1;
            }
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
    else if (((strcmp(arguments[0], "show") == 0 && flag == 2) || (((strcmp(arguments[0], "st") == 0 || strcmp(arguments[0], "sr") == 0 || strcmp(arguments[0], "sc") == 0) && flag == 1))) && state != notreg)
    {

        if (strcmp(arguments[1], "topology") == 0 || strcmp(arguments[0], "st") == 0)
        {
            print_topology(neighbours);
            return 0;
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
        //de-register
        leave_server(netID, sockfd, argv[1], argv[2]);

        //fechar todos os fds e addrinfo
        for(i = 0; i < *n_neighbours + 1; i++)
            close(neighbours[i].sockfd);

        freeaddrinfo(neighbours[0].node_info);

        //atualizar o estado
        state = notreg;

        return 0;
    }
    else if (strcmp(arguments[0], "exit") == 0 && flag == 1)
    {
        if(state == reg || state == lonereg)
        {
            //de-register
            leave_server(netID, sockfd, argv[1], argv[2]);

            //fechar todos os fds e addrinfo
            for(i = 0; i < *n_neighbours + 1; i++)
            {
                if(close(neighbours[i].sockfd) == -1)
                {
                    return -1;
                }
            }
            freeaddrinfo(neighbours[0].node_info);
        }
        state = exiting;
        return 1;
    }

    //se não for encontrado qualquer comando da lista
    printf("Invalid command\n");
    return -1;
}

//Ordena um array para fazer uma lista de opções para conectar
int* random_neighbour(int n_nodes, int* shuffle)
{
    int i, temp, randIndex;

    for(i = 0; i < n_nodes; i++) 
    {
        shuffle[i] = i;
    }

    for(i = 0; i < n_nodes; i++) 
    {
        temp = shuffle[i];
        randIndex = rand() % n_nodes;

        shuffle[i] = shuffle[randIndex];
        shuffle[randIndex] = temp;
    }

    return shuffle;
}

/*
Validar comando enviado por TCP
Return: 
    1 - NEW
    2 - EXTERN
    3 - ADVERTISE
    4 - WITHDRAW
    -1 - erro ou mensagem não existe no protocolo
*/
int validate_messages(char* mail)
{
    int flag = 0;
    char buffer[BUF_SIZE];
    char arguments[5][BUF_SIZE];

    sscanf(mail, "%s", buffer);

    if(strcmp(buffer, "NEW") == 0)
    {
        flag = sscanf(mail, "%s %s %s\n", arguments[0], arguments[1], arguments[2]);
        if(flag != 3)
        {
            return -1;
        }
        if(validar_IPv4(arguments[1]) == -1 || validar_port(arguments[2]) == -1)
        {
            return -1;
        }
        return 1;
    }
    if(strcmp(buffer, "EXTERN") == 0)
    {
        flag = sscanf(mail, "%s %s %s\n", arguments[0], arguments[1], arguments[2]);
        if(flag != 3)
        {
            return -1;
        }
        if(validar_IPv4(arguments[1]) == -1 || validar_port(arguments[2]) == -1)
        {
            return -1;
        }
        //comando conhecido
        return 2;
    }
    if(strcmp(buffer, "ADVERTISE") == 0)
    {
        //comando conhecido
        return 3;
    }
    if(strcmp(buffer, "WITHDRAW") == 0)
    {
        //comando conhecido
        return 4;
    }

    return -1;
}