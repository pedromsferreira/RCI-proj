#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#define BUF_SIZE 1024

//Variável única
typedef struct nodes{
    char IP[16];
    char TCP[6];
}nodes;

//Global variables
enum {reg, notreg, exiting} state;
struct addrinfo *server_info;
int n_nodes;
nodes nodeslist[3000];

//Headers do ficheiro network.c
void UDP_socket(int argc, char* IP, char* UDP, int* sockfd);
void state_machine(int argc, char* argv[]);
void wait_for_answer(int sockfd);
int ask_list(char *netID, int sockfd);
int random_picker(int list_number);

#endif