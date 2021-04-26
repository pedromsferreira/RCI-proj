#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "defines.h"
#include "validation.h"
#include "commands.h"
#include "TCP.h"


/*
Initialize socket for TCP connection with client
Return:
    fd if successfull
    -1 if something went wrong
*/
int TCP_client(char* IP, char* TCP, struct addrinfo* node_info)
{

    int sockfd, flag = 0;
    struct addrinfo hints;
    struct addrinfo *res;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    flag = getaddrinfo(IP, TCP, &hints, &res);
    if (flag != 0)
    {
        printf("Error: %s\n", gai_strerror(flag));
        return -1;
    }

    flag = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (flag != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    node_info = res;
    return sockfd;
}

int TCP_server(char* TCP, neighbour* neighbours)
{
    int sockfd, flag;
    struct addrinfo hints;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    flag = getaddrinfo(NULL, TCP, &hints, &neighbours[0].node_info);
    if (flag != 0)
    {
        printf("Error: %s\n", gai_strerror(flag));
        return -1;
    }

    flag = bind(sockfd, neighbours[0].node_info->ai_addr, neighbours[0].node_info->ai_addrlen);
    if (flag != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    flag = listen(sockfd,5);
    if (flag != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }


    return sockfd;
}

/*
Send a message through TCP
Return:
    0 if successfull
    -1 if something went wrong
*/
int write_to_someone(char* nodeIP, char* nodeTCP, int nodefd, char* command)
{
    int left = 0, flag = 0;
    char *ptr, buffer[BUF_SIZE];

    sprintf(buffer, "%s %s %s\n", command, nodeIP, nodeTCP);
    left = strlen(buffer);
    ptr = buffer;

    while(left > 0)
    {
        flag = write(nodefd, ptr, left);
        //erro na escrita ou closed by peer
        if(flag == -1 || flag == 0)
        {
            return -1;
        }

        left -= flag;
        ptr += flag;
    }
    //enviado com sucesso
    return 0;
}

int TCP_command_hub(int flag, neighbour* neighbours, char* mail, int n_neighbours)
{
    switch(flag)
    {
        case 1 :
            execute_NEW(neighbours, mail, n_neighbours);
            break;

        case 2 :
            execute_EXTERN(neighbours, mail);
            break;

        case 3 :
            break;

        case 4 :
            break;
    }

    return 0;
}

/*
Read a message through TCP
Return:
    flag corresponding to command executed
    -1 if something went wrong
*/
int read_from_someone(neighbour* placeholder, int ready_index, int n_neighbours)
{
    int received = 0, flag = 0;
    char *ptr, *ptr2;

    //Pointer no início da string
    ptr = placeholder[ready_index].mail_sent;
    ptr2 = placeholder[ready_index].mail_sent;

    //Guardar mensagem na struct, tendo em conta se já tem lá informação
    received = read(placeholder[ready_index].sockfd, ptr + strlen(placeholder[ready_index].mail_sent), BUF_SIZE*4);
    if(received == -1)
        return -1;
    
    //não leu nada adicional, return
    if(received == 0)
        return 0;
    
    //buffer overflow
    if(strlen(placeholder[ready_index].mail_sent) >= BUF_SIZE*4-1 && strstr(placeholder[ready_index].mail_sent, "\n") == NULL)
    {
        placeholder[ready_index].mail_sent[0] = '\0';
        return 0;
    }


    //Encontrar "\n" na mensagem
    if(received > 0)
    {
        while(1) //Enquanto houver "\n", estará sempre à procura de mais comandos
        {   
            ptr2 = strstr(placeholder[ready_index].mail_sent, "\n");
            if(ptr2 == NULL)
                return flag;

            //Validar o argumento
            flag = validate_messages(placeholder[ready_index].mail_sent);

            //Executar o comando 
            TCP_command_hub(flag, placeholder, placeholder[ready_index].mail_sent, n_neighbours);
                
            //Atualiza a string para estar à frente do "\n"
            ptr2 += 1;
            strcpy(placeholder[ready_index].mail_sent, ptr2);
        }
    }
    
    return flag;
}

int accept_connection(int listenfd, neighbour neighbours)
{
    int newfd;

    newfd = accept(listenfd, neighbours.node_info->ai_addr, &neighbours.node_info->ai_addrlen);
    if(newfd == -1)
    {
        return -1;
    }
    printf("\nO gigante está entrando\n");
    printf("\nO gigante já entrou\n");

    return newfd;
}
