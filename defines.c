#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "defines.h"

/******************************************************************************
* Returns number of the biggest fd
*
* Returns: (int)
*   maxfd if normal procedure
******************************************************************************/
int max(neighbour *fd_list, int n)
{
    int i, maxfd = 0;

    for (i = 0; i <= n; i++)
    {
        if (maxfd < fd_list[i].sockfd)
        {
            maxfd = fd_list[i].sockfd;
        }
    }

    return maxfd;
}

/******************************************************************************
* Accept a connection from client through TCP
*
* Returns: (int)
*   fd if normal procedure
*   -1 if connection is missing
******************************************************************************/
int find_ID_index_in_struct_neighbours(neighbour *neighbours, expedition_table *table, int *n_neighbours, int ready_index)
{
    int j;

    for (j = 1; j < *n_neighbours + 2; j++)
    {
        //Ignorar nó de recuperação
        if (j == 2)
        {
            continue;
        }

        //Sockets têm de corresponder
        if (table->sockfd[ready_index] == neighbours[j].sockfd)
        {
            //return
            return j;
        }
    }
    return -1;
}

/******************************************************************************
* Accept a connection from client through TCP
*
* Returns: (int)
*   fd if normal procedure
*   -1 if connection is missing
******************************************************************************/
int find_ID_index_in_expedition_table(char* ID, expedition_table* table)
{
    int i;

    //Procura pelo índice do sockfd na expedition table
    for (i = 1; i <= table->n_id; i++)
    {
        //Determinar o índice do sockfd na expedition table
        if (strcmp(table->id[i], ID) == 0)
        {
            return i;
        }
    }
    return -1;
}