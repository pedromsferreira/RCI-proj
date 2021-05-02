#ifndef DEFINES_H_INCLUDED
#define DEFINES_H_INCLUDED

#define BUF_SIZE 1024
#define MAX_NODES 512
#define MAX_NEIGHBOURS 128
#define MAX_OBJECTS 64
#define WAIT_TIME 5
#define MAX_WAIT 500

//Variáveis únicas
typedef struct nodes{
    char IP[16];
    char TCP[6];
}nodes;

//Estrutura de dados utilizada na topologia
typedef struct neighbour{
    nodes node;
    int sockfd; 
    struct addrinfo *node_info;
    char mail_sent[BUF_SIZE*4];
}neighbour;

//Estrutura de dados utilizada no encaminhamento
typedef struct expedition_table{
    int n_id;
    int sockfd[MAX_NODES];
    char id[MAX_NODES][BUF_SIZE];
}expedition_table;

//Estrutura de dados utilizada na pesquisa de objetos
typedef struct object_search{
    char objects[MAX_OBJECTS][BUF_SIZE]; //Guardar objetos (nomes) no formato id.subname
    int n_objects; //Número de objetos


    char ID_return[MAX_OBJECTS][BUF_SIZE]; //ID para o qual se manda a mensagem de retorno
    char object_return[MAX_OBJECTS][BUF_SIZE]; //Objeto correspondente ao ID para retornar
    int n_return; //Número de objetos à espera de retorno
    clock_t timer[MAX_OBJECTS];

    //Posições definidas:
    //  0 - posição mais recente
    //  1 - posição menos recente
    //  id.subnome
    char cache_objects[2][BUF_SIZE]; //Cache para guardar os últimos 2 objetos recebidos
}object_search;

//Global variables
enum {reg, lonereg, notreg, leaving, exiting} state;
struct addrinfo *server_info;

int max(neighbour *fd_list, int n);
int find_ID_index_in_struct_neighbours(neighbour* neighbours, expedition_table* table, int* n_neighbours, int ready_index);
int find_ID_index_in_expedition_table(char* ID, expedition_table* table);

#endif