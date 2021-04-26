#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

int join_complicated(char *netID, char *nodeID, int sock_server, char *nodeIP, char *nodeTCP, neighbour* neighbours, int* n_neighbours/*, expedition_table* table*/);
int join_simple(char *netID, char *nodeID, char* bootIP, char* bootTCP);
void print_topology(neighbour* neighbours);
int leave_server(char *netID, int sock_server, char *nodeIP, char *nodeTCP);
void execute_NEW(neighbour* neighbours, char* mail_sent, int n_neighbours);
void execute_EXTERN(neighbour* neighbours, char* mail_sent);

#endif