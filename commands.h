#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

int join_complicated(char *netID, char *nodeID, int sock_server, char *nodeIP, char *nodeTCP, neighbour* neighbours, int* n_neighbours/*, expedition_table* table*/);
int join_simple(char *netID, char *nodeID, char* bootIP, char* bootTCP, int sock_server, char *nodeIP, char *nodeTCP, neighbour* neighbours, int* n_neighbours/*, expedition_table* table*/);
void print_topology(neighbour* neighbours);
int leave_server(char *netID, int sock_server, char *nodeIP, char *nodeTCP);
void execute_NEW(neighbour* neighbours, char* mail_sent, int n_neighbours);
void execute_EXTERN(neighbour* neighbours, char* mail_sent);
void execute_ADVERTISE(neighbour* neighbours, char* mail_sent);
int close_all_sockets(int n_neighbours, neighbour* neighbours);
int close_socket(int* n_neighbours, neighbour* neighbours, int chosen_index);
int close_listen(neighbour* neighbours);

#endif