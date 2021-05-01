#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

int join_complicated(char *netID, char *nodeID, int sock_server, char *nodeIP, char *nodeTCP, neighbour *neighbours, int *n_neighbours, expedition_table *table, object_search *FEDEX);
int join_simple(char *netID, char *nodeID, char *bootIP, char *bootTCP, int sock_server, char *nodeIP, char *nodeTCP, neighbour *neighbours, int *n_neighbours, expedition_table *table, object_search *FEDEX);
void create_subname(char *ID, char *subname, object_search *FEDEX);
void clean_objects(char *ID, char *subname, object_search *FEDEX, int option);
void start_search_for_object(char *name, object_search *FEDEX, expedition_table *table, neighbour *neighbours, int *n_neighbours);
void print_topology(neighbour* neighbours);
void print_routing(expedition_table table);
void print_cache(object_search* FEDEX);
void print_objects(object_search *FEDEX);
int leave_server(char *netID, int sock_server, char *nodeIP, char *nodeTCP);
void execute_NEW(neighbour *neighbours, char *mail_sent, int *n_neighbours, int ready_index, expedition_table* table);
void execute_EXTERN(neighbour *neighbours, char *mail_sent, int ready_index);
void execute_ADVERTISE(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table* table, int* n_neighbours);
void execute_WITHDRAW(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours);
void execute_INTEREST(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours, object_search *FEDEX);
void execute_DATA(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours, object_search *FEDEX);
void execute_NODATA(neighbour *neighbours, char *mail_sent, int ready_index, expedition_table *table, int *n_neighbours, object_search *FEDEX);
int close_all_sockets(int n_neighbours, neighbour *neighbours, expedition_table *table, object_search* FEDEX);
int close_socket(int *n_neighbours, neighbour *neighbours, int chosen_index, expedition_table* table);
int close_listen(neighbour *neighbours, expedition_table *table, object_search* FEDEX);
void reset_table(expedition_table* table);
void reset_objects(object_search *FEDEX);
void update_line_return_FEDEX(object_search *FEDEX, int index);
void store_in_cache(object_search* FEDEX, char* ID_subname);

#endif
    