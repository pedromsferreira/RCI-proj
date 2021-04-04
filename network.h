#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

//Global variables
enum {reg, notreg, exiting} state;
int sockfd;
struct addrinfo hints, *server_info;

//Headers do ficheiro network.c
void UDP_socket(int argc, char* IP, char* UDP);
void state_machine(int argc, char* argv[]);
void wait_for_answer();
void ask_list(char *netID);

#endif