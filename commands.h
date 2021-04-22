#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

int join_complicated(char *netID, char *nodeID, int sock_server, char *nodeIP, char *nodeTCP, nodes topology[2], nodes* nodeslist);
int join_simple(char *netID, char *nodeID, char* bootIP, char* bootTCP);
int leave_server(char *netID, int sock_server, char *nodeIP, char *nodeTCP);

#endif