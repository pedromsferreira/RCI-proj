#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

int join_complicated(char *netID, char *nodeID, int sock_server, char *nodeIP, char *nodeTCP, neighbour* neighbours, int* n_neighbours, expedition_table* table);
int join_simple(char *netID, char *nodeID, char* bootIP, char* bootTCP, int sock_server, char *nodeIP, char *nodeTCP, neighbour* neighbours, int* n_neighbours, expedition_table* table);
void print_topology(neighbour* neighbours);
void print_routing(expedition_table table);
int leave_server(char *netID, int sock_server, char *nodeIP, char *nodeTCP);
void execute_NEW(neighbour *neighbours, char *mail_sent, int *n_neighbours, int ready_index, expedition_table* table);
void execute_EXTERN(neighbour *neighbours, char *mail_sent, int ready_index);
void execute_ADVERTISE(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table* table, int* n_neighbours);
void execute_WITHDRAW(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours);
int close_all_sockets(int n_neighbours, neighbour *neighbours, expedition_table* table);
int close_socket(int *n_neighbours, neighbour *neighbours, int chosen_index, expedition_table* table);
int close_listen(neighbour *neighbours, expedition_table* table);
void reset_table(expedition_table* table);

#endif
    