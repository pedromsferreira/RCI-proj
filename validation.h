#ifndef VALIDATION_H_INCLUDED
#define VALIDATION_H_INCLUDED

//Headers do ficheiro validation.c
void instructions();
int validar_IPv4 (char* IPv4);
int validar_port(char* port);
void validate_start(int argc, char* argv[]);
int user_interface(int sockfd, char* argv[], neighbour* neighbours, int* n_neighbours, char* netID, expedition_table* table, object_search *FEDEX);
int* random_neighbour(int n_neighbours, int* shuffle);
int validate_messages(char* mail);
int separate_ID_subname(char* ID_subname, char* ID, char* subname, expedition_table* table);

#endif