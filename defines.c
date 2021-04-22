#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "defines.h"

int max(neighbour *fd_list, int n)
{
    int i, maxfd = 0;

    for(i = 0; i < n; i++)
    {
        if(maxfd < fd_list[i].sockfd)
        {
            maxfd = fd_list[i].sockfd;
        }
    }

    return maxfd;
}