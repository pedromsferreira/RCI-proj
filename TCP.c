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
#include "network.h"
#include "TCP.h"

/*
Initialize socket for TCP connection with client
Return:
    fd if successfull
    -1 if something went wrong
*/
int TCP_client(char *IP, char *TCP, struct addrinfo *node_info)
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

int TCP_server(char *TCP, neighbour *neighbours)
{
    int sockfd, flag;
    struct addrinfo hints;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
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

    flag = listen(sockfd, 5);
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
int write_to_someone(char *nodeIP, char *nodeTCP, neighbour* neighbours, char *command, int destination, int* n_neighbours)
{
    int left = 0, flag = 0;
    char *ptr, buffer[BUF_SIZE];

    sprintf(buffer, "%s %s %s\n", command, nodeIP, nodeTCP);
    left = strlen(buffer);
    ptr = buffer;

    while (left > 0)
    {
        flag = write(neighbours[destination].sockfd, ptr, left);
        //erro na escrita
        if (flag == -1)
        {
            backup_plan(destination, n_neighbours, neighbours);
            return -1;
        }

        left -= flag;
        ptr += flag;
    }
    //enviado com sucesso
    return 0;
}

int TCP_command_hub(int flag, neighbour *neighbours, char *mail, int n_neighbours)
{
    switch (flag)
    {
    case 1:
        execute_NEW(neighbours, mail, n_neighbours);
        break;

    case 2:
        execute_EXTERN(neighbours, mail);
        break;

    case 3:
        execute_ADVERTISE(neighbours, mail);
        break;

    case 4:
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
int read_from_someone(neighbour *placeholder, int ready_index, int *n_neighbours)
{
    int received = 0, flag = 0;
    char *ptr, *ptr2;

    //Pointer no início da string
    ptr = placeholder[ready_index].mail_sent;
    ptr2 = placeholder[ready_index].mail_sent;

    //Guardar mensagem na struct, tendo em conta se já tem lá informação
    received = read(placeholder[ready_index].sockfd, ptr + strlen(placeholder[ready_index].mail_sent), BUF_SIZE * 4);

    //socket fechou ou algo de mal aconteceu
    if (received == 0 || received == -1)
    {
        backup_plan(ready_index, n_neighbours, placeholder);
        return received;
    }

    //buffer overflow
    if (strlen(placeholder[ready_index].mail_sent) >= BUF_SIZE * 4 - 1 && strstr(placeholder[ready_index].mail_sent, "\n") == NULL)
    {
        placeholder[ready_index].mail_sent[0] = '\0';
        return 0;
    }

    //Encontrar "\n" na mensagem
    if (received > 0)
    {
        while (1) //Enquanto houver "\n", estará sempre à procura de mais comandos
        {
            ptr2 = strstr(placeholder[ready_index].mail_sent, "\n");
            if (ptr2 == NULL)
                return flag;

            //Validar o argumento
            flag = validate_messages(placeholder[ready_index].mail_sent);

            //Executar o comando
            TCP_command_hub(flag, placeholder, placeholder[ready_index].mail_sent, *n_neighbours);

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
    if (newfd == -1)
    {
        return -1;
    }
    printf("\nO gigante está entrando\n");
    printf("\nO gigante já entrou\n");

    return newfd;
}

/*
Promotes internal neighbour to EXTERN
Return:
    0 if someone is promoted
    -1 if there's no neighbours able to promote
WARNING: n_neighbours does not count with previous EXTERN
*/
int promote_to_EXTERN(neighbour *neighbours, int *n_neighbours)
{
    int i;
    //Se tem internos
    if (*n_neighbours > 0)
    {
        while (*n_neighbours > 0)
        {
            if (exchange_contacts(neighbours, neighbours[3].sockfd, n_neighbours, 3) == 0)
            {
                neighbours[1] = neighbours[3];

                if (*n_neighbours > 1)
                {
                    //reordenar vizinhos internos
                    for (i = 3; i < *n_neighbours + 3; i++)
                    {
                        neighbours[i] = neighbours[i + 1];
                    }
                    //atualizar vizinhos internos com EXTERN
                }
                return 0;
            }
            close_socket(n_neighbours, neighbours, 3);
        }
    }
    return -1;
}

int exchange_contacts(neighbour *neighbours, int sockfd, int *n_neighbours, int index)
{

    //Enviar mensagem de presença ao nó externo com o nosso IP e TCP
    if (write_to_someone(neighbours[0].node.IP, neighbours[0].node.TCP, neighbours, "NEW", index, n_neighbours) == -1)
    {
        close_socket(n_neighbours, neighbours, index);
        return -1;
    }

    //Confirmar presença de um buffer para leitura
    if (wait_for_answer(sockfd, 500) == -1)
    {
        close_socket(n_neighbours, neighbours, index);
        return -1;
    }

    //Receber mensagem de presença do nó externo com o IP e TCP do vizinho de recuperação deles, e ADVERTISEs
    if (read_from_someone(neighbours, index, n_neighbours) == -1)
    {
        close_socket(n_neighbours, neighbours, index);
        return -1;
    }
    return 0;
}

int update_RECOVERY(int *n_neighbours, neighbour *neighbours)
{
    int i;

    for (i = 3; i < *n_neighbours + 3; i++)
    {
        //Enviar mensagem de EXTERN
        if (write_to_someone(neighbours[1].node.IP, neighbours[1].node.TCP, neighbours, "EXTERN", i, n_neighbours) == -1)
        {
            close_socket(n_neighbours, neighbours, i);
            i--;
        }
    }
    return 0;
}

int backup_plan(int ready_index, int* n_neighbours, neighbour* placeholder)
{
    if (close_socket(n_neighbours, placeholder, ready_index) == -1)
    {
        return -1;
    }
    //promover vizinho de recuperação a externo
    if (ready_index == 1 && *n_neighbours >= 0 && placeholder[2].sockfd != placeholder[0].sockfd)
    {
        placeholder[1] = placeholder[2];
        //Abrir conexão TCP com o antigo nó de recuperação, agora externo
        placeholder[1].sockfd = TCP_client(placeholder[1].node.IP, placeholder[1].node.TCP, placeholder[1].node_info);

        if (write_to_someone(placeholder[0].node.IP, placeholder[0].node.TCP, placeholder, "NEW", 1, n_neighbours) == -1)
        {
            if (close_socket(n_neighbours, placeholder, 2) == -1)
                return -1;
            if (promote_to_EXTERN(placeholder, n_neighbours) == -1)
            {
                placeholder[1] = placeholder[0];
                placeholder[2] = placeholder[0];
                state = lonereg;
            }
            return 0;
        }

        //Confirmar presença de um buffer para leitura
        if (wait_for_answer(placeholder[1].sockfd, 5) == -1)
        {
            if (close_socket(n_neighbours, placeholder, 2) == -1)
                return -1;
            if (promote_to_EXTERN(placeholder, n_neighbours) == -1)
            {
                placeholder[1] = placeholder[0];
                placeholder[2] = placeholder[0];
                state = lonereg;
            }
            return 0;
        }

        if (read_from_someone(placeholder, 1, n_neighbours) == -1)
        {
            if (close_socket(n_neighbours, placeholder, 2) == -1)
                return -1;
            if (promote_to_EXTERN(placeholder, n_neighbours) == -1)
            {
                placeholder[1] = placeholder[0];
                placeholder[2] = placeholder[0];
                state = lonereg;
            }
            return 0;
        }
        //se recuperação for promovido a externo, incrementar vizinhos
        *n_neighbours+=1;
    }
    //se o vizinho de recuperação é o prório
    else if (ready_index == 1 && *n_neighbours > 0 && placeholder[2].sockfd == placeholder[0].sockfd)
    {
        if (promote_to_EXTERN(placeholder, n_neighbours) == -1)
        {
            placeholder[1] = placeholder[0];
            placeholder[2] = placeholder[0];
            state = lonereg;
        }
    }
    //se o vizinho de recuperação é o prório e não tem vizinhos internos
    else if (ready_index == 1 && *n_neighbours == 0 && placeholder[2].sockfd == placeholder[0].sockfd)
    {
        placeholder[1] = placeholder[0];
        state = lonereg;
    }
    return 0;
}