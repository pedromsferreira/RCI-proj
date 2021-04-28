#ifndef UDP_H_INCLUDED
#define UDP_H_INCLUDED

//Headers do ficheiro UDP.c
int UDP_socket(int argc, char *IP, char *UDP);
int ask_list(char *netID, int sockfd, nodes* nodeslist, int* n_nodes);

#endif