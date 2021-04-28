#ifndef TCP_H_INCLUDED
#define TCP_H_INCLUDED

//Headers do ficheiro TCP.c
int TCP_client(char* IP, char* TCP, struct addrinfo* node_info);
int TCP_server(char* TCP, neighbour* neighbours);
int write_to_someone(char *nodeIP, char *nodeTCP, neighbour* neighbours, char *command, int destination, int* n_neighbours);
int TCP_command_hub(int flag, neighbour *neighbours, char *mail, int n_neighbours, int ready_index);
int read_from_someone(neighbour* placeholder, int ready_index, int* n_neighbours);
int accept_connection(int listenfd, neighbour neighbours);
int exchange_contacts(neighbour* neighbours, int sockfd, int* n_neighbours, int index);
int promote_to_EXTERN(neighbour* neighbours, int* n_neighbours);
int update_RECOVERY(int* n_neighbours, neighbour* neighbours);
int backup_plan(int ready_index, int* n_neighbours, neighbour* placeholder);
void inform_internal_newEXTERN(int* n_neighbours, neighbour* neighbours);

#endif