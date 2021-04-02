#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "validation.h"

//Instruções para dar input corretamente após erro
void instructions(){
    printf("To start this program, your command must be formatted like this:\n\n");
    printf("ndn <IP> <TCP> <regIP> <regUDP>\n\n");
    return;
} 

//Validar IPv4 introduzido
int validar_IPv4 (char* IPv4){
    int flag = 0;
    char buf[sizeof(struct in_addr)];

    
    flag = inet_pton(AF_INET, IPv4, buf);
    if(flag != 1){
        if(flag == 0)
            printf("Error found: IP inserted is not valid\n");
        if(flag == -1)
            printf("Error found: %s\n", strerror(errno));
        return 1;
    }
    return 0;
}

//Validar port number introduzido
int validar_port(char* port){
    int port_number = atoi(port);


    if(port_number>65536 || port_number<1){
        printf("Error found: port number inserted is not valid\n");
        return 1;
    }
    return 0; 
}

//Validação do comando que inicia o programa
void validate_start(int argc, char* argv[]){
    int flag = 0;


    if(argc < 3 || argc == 4){
        printf("Error found: %s\n", strerror(EINVAL));
        instructions();
        exit(1);
    }
    if(argc > 5){
        printf("Error found: %s\n", strerror(E2BIG));
        instructions();
        exit(1);
    }

    flag += validar_IPv4(argv[1]);
    flag += validar_port(argv[2]);

    if(argc > 3){
        flag += validar_IPv4(argv[3]);
        flag += validar_port(argv[4]);
    }
    
    if(flag != 0){
        instructions();
        exit(1);
    }
    return;
}

int user_interface(){

    return 0;
}