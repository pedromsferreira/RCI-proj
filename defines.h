#ifndef DEFINES_H_INCLUDED
#define DEFINES_H_INCLUDED

#define BUF_SIZE 1024
#define MAX_NODES 600
#define MAX_NEIGHBOURS 128

//Variáveis únicas
typedef struct nodes{
    char IP[16];
    char TCP[6];
}nodes;

typedef struct neighbour{
    nodes node;
    int sockfd; 
    struct addrinfo *node_info;
    char mail_sent[BUF_SIZE*4];
}neighbour;

/*
typedef struct expedition_table{
    int n_id;
    
}expedition_table;
*/

//Global variables
enum {reg, lonereg, notreg, exiting} state;
struct addrinfo *server_info;

int max(neighbour *fd_list, int n);

#endif