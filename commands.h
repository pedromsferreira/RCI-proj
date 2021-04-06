#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

int join_complicated(char *netID, char *nodeID, int sock_server);
int join_simple(char *netID, char *nodeID, char* bootIP, char* bootTCP);

#endif