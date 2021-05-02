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

/******************************************************************************
* Instructions for the user
******************************************************************************/
void instructions()
{
    printf("To start this program, your command must be formatted like this:\n\n");
    printf("ndn <IP> <TCP> <regIP> <regUDP>\n\n");
    return;
}

/******************************************************************************
* Validate IPv4 introduced
*
* Returns: (int)
*    0 - when IP is valid
*    -1 - when IP is invalid
******************************************************************************/
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

/******************************************************************************
* Validate inserted port number
*
* Returns: (int)
*    0 - if port number is valid
*    1 - if port number is invalid
******************************************************************************/
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

/******************************************************************************
* Validate command that starts program
******************************************************************************/
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

/******************************************************************************
* Validate command written in stdin during program
*
* Returns: (int)
*    1 - Program closing
*    0 - Command known e well executed
*    -1 - ERROR
******************************************************************************/
int user_interface(int sockfd, char *argv[], neighbour *neighbours, int *n_neighbours, char *netID, expedition_table *table, object_search *FEDEX)
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
            error = join_complicated(arguments[1], arguments[2], sockfd, argv[1], argv[2], neighbours, n_neighbours, table, FEDEX);
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
            error = join_simple(arguments[1], arguments[2], arguments[3], arguments[4], sockfd, argv[1], argv[2], neighbours, n_neighbours, table, FEDEX);
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
    }
    else if (strcmp(arguments[0], "create") == 0 && flag == 2 && state != notreg)
    {
        create_subname(table->id[0], arguments[1], FEDEX);
        return 0;
    }
    else if (strcmp(arguments[0], "clear") == 0 && flag == 2 && state != notreg)
    {
        if (strcmp(arguments[1], "all") == 0)
        {
            clean_objects(table->id[0], arguments[1], FEDEX, 0);
            return 0;
        }
        clean_objects(table->id[0], arguments[1], FEDEX, 1);
        return 0;
    }
    else if (strcmp(arguments[0], "get") == 0 && flag == 2 && state != notreg)
    {
        start_search_for_object(arguments[1], FEDEX, table, neighbours, n_neighbours);
        return 0;
    }
    else if (((strcmp(arguments[0], "show") == 0 && flag == 2) || (((strcmp(arguments[0], "st") == 0 || strcmp(arguments[0], "sr") == 0 || strcmp(arguments[0], "sc") == 0 || strcmp(arguments[0], "so") == 0) && flag == 1))) && state != notreg)
    {

        if ((flag == 2  && strcmp(arguments[1], "topology") == 0 ) || (flag == 1 && strcmp(arguments[0], "st") == 0))
        {
            print_topology(neighbours);
            return 0;
        }

        else if ((flag == 2  && strcmp(arguments[1], "routing") == 0) || (flag == 1 && strcmp(arguments[0], "sr") == 0))
        {
            print_routing(*table);
            return 0;
        }

        else if ((flag == 2 && strcmp(arguments[1], "cache") == 0) || (flag == 1 && strcmp(arguments[0], "sc") == 0))
        {
            print_cache(FEDEX);
            return 0;
        }
        else if ((flag == 2 && strcmp(arguments[1], "objects") == 0) || (flag == 1 && strcmp(arguments[0], "so") == 0))
        {
            print_objects(FEDEX);
            return 0;
        }
    }
    else if (strcmp(arguments[0], "leave") == 0 && flag == 1 && state != notreg)
    {
        if(leave_protocol(netID, sockfd, argv, n_neighbours, neighbours, table, FEDEX) == -1)
            return -1;

        //atualizar o estado
        *n_neighbours = 0;
        state = notreg;

        return 0;
    }
    else if (strcmp(arguments[0], "exit") == 0 && flag == 1)
    {
        if (state == reg || state == lonereg)
        {
            if(leave_protocol(netID, sockfd, argv, n_neighbours, neighbours, table, FEDEX) == -1)
                return -1;
        }
        //atualizar o estado
        *n_neighbours = 0;
        state = exiting;
        return 1;
    }

    //se não for encontrado qualquer comando da lista
    printf("Invalid command. Check README for valid commands\n");
    return -1;
}

/******************************************************************************
* Sorts an array to make a list of options to connect to
*
* Returns: (int)
*   shuffle if successfull
******************************************************************************/
int *random_neighbour(int n_nodes, int *shuffle)
{
    int i, temp, randIndex;

    for (i = 0; i < n_nodes; i++)
    {
        shuffle[i] = i;
    }

    for (i = 0; i < n_nodes; i++)
    {
        temp = shuffle[i];
        randIndex = rand() % n_nodes;

        shuffle[i] = shuffle[randIndex];
        shuffle[randIndex] = temp;
    }

    return shuffle;
}

/******************************************************************************
* Validate command sent by TCP protocol
*
* Returns: (int)
*    1 - NEW
*    2 - EXTERN
*    3 - ADVERTISE
*    4 - WITHDRAW
*    5 - INTEREST
*    6 - DATA
*    7 - NODATA
*   -1 - ERROR or message not part of protocol
******************************************************************************/
int validate_messages(char *mail)
{
    int flag = 0;
    char buffer[BUF_SIZE];
    char arguments[5][BUF_SIZE];

    sscanf(mail, "%s", buffer);

    if (strcmp(buffer, "NEW") == 0)
    {
        flag = sscanf(mail, "%s %s %s\n", arguments[0], arguments[1], arguments[2]);
        if (flag != 3)
        {
            return -1;
        }
        if (validar_IPv4(arguments[1]) == -1 || validar_port(arguments[2]) == -1)
        {
            return -1;
        }
        return 1;
    }
    else if (strcmp(buffer, "EXTERN") == 0)
    {
        flag = sscanf(mail, "%s %s %s\n", arguments[0], arguments[1], arguments[2]);
        if (flag != 3)
        {
            return -1;
        }
        if (validar_IPv4(arguments[1]) == -1 || validar_port(arguments[2]) == -1)
        {
            return -1;
        }
        //comando conhecido
        return 2;
    }
    else if (strcmp(buffer, "ADVERTISE") == 0)
    {
        flag = sscanf(mail, "%s %s\n", arguments[0], arguments[1]);
        if (flag != 2)
        {
            return -1;
        }
        return 3;
    }
    else if (strcmp(buffer, "WITHDRAW") == 0)
    {
        flag = sscanf(mail, "%s %s\n", arguments[0], arguments[1]);
        if (flag != 2)
        {
            return -1;
        }
        return 4;
    }
    else if (strcmp(buffer, "INTEREST") == 0)
    {
        flag = sscanf(mail, "%s %s\n", arguments[0], arguments[1]);
        if (flag != 2)
        {
            return -1;
        }
        return 5;
    }
    else if (strcmp(buffer, "DATA") == 0)
    {
        flag = sscanf(mail, "%s %s\n", arguments[0], arguments[1]);
        if (flag != 2)
        {
            return -1;
        }
        return 6;
    }
    else if (strcmp(buffer, "NODATA") == 0)
    {
        flag = sscanf(mail, "%s %s\n", arguments[0], arguments[1]);
        if (flag != 2)
        {
            return -1;
        }
        return 7;
    }

    return -1;
}

/******************************************************************************
* Separate ID from subname, comparing to ID in table struct
* 
*
* Returns: (int)
*   0 if successful
*  -1 if failure
******************************************************************************/
int separate_ID_subname(char *ID_subname, char *ID, char *subname, expedition_table *table)
{
    char *ptr;

    for (int i = 0; i < table->n_id; i++)
    {
        if (strncmp(table->id[i], ID_subname, strlen(table->id[i])) == 0)
        {
            strcpy(ID, table->id[i]);
            ptr = ID_subname;
            ptr += strlen(ID) + 1; //avançar ponteiro para o início do subname
            strcpy(subname, ptr);
            return 0;
        }
    }
    return -1;
}