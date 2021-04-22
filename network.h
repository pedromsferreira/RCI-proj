#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

//Headers do ficheiro network.c
void state_machine(int argc, char* argv[]);
int wait_for_answer(int sockfd);

#endif