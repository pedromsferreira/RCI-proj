#ifndef DEFINES_H_INCLUDED
#define DEFINES_H_INCLUDED

#define BUF_SIZE 1024
#define MAX_NODES 600

//Variáveis únicas
typedef struct nodes{
    char IP[16];
    char TCP[6];
    struct addrinfo *node_info;
}nodes;

typedef struct neighbour{
    char IP[16];
    char TCP[6];
    int sockfd;
}neighbour;

//Global variables
enum {reg, notreg, exiting} state;
struct addrinfo *server_info;
int n_nodes;

#endif