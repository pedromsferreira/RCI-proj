#ifndef TCP_H_INCLUDED
#define TCP_H_INCLUDED

//Headers do ficheiro TCP.c
int TCP_client(char* IP, char* TCP, struct addrinfo* node_info);
int TCP_server(char* TCP, struct addrinfo* server_addr);
int write_to_someone(char* nodeIP, char* nodeTCP, int nodefd, char* command);
int read_join(neighbour placeholder);

#endif