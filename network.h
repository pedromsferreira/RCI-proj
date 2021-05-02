#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

//Headers do ficheiro network.c
void state_machine(int argc, char **argv);
int wait_for_answer(int sockfd, int seconds);
int insert_ID_in_table (expedition_table* table, int sockfd, char* ID);
void remove_ID_from_table(expedition_table* table, char* ID);
void remove_socket_from_table(expedition_table* table, int sockfd);

#endif