#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

int main(int argc, char argv[]){
    //Variables

    //code
    //validation
    if(argc<3){
        printf("Error found: %s\n", strerror(EINVAL));
        exit(1);
    }
    if(argc>5){
        printf("Error found: %s\n", strerror(E2BIG));
        exit(1);
    }
    if(argc==3){
        serv_IP = "193.136.138.142";
        serv_UDP_port = "59000";
    }


    strcpy(local_IP,argv[1]);
    strcpy(local_TCP_port,argv[2]);
    strcpy(serv_IP,argv[3]);
    strcpy(serv_UDP_port,argv[4]);

    return 0;
}


logout(){


}

int validar_IPv4 (*char IPv4){
    int flag = 0;
    char buf[sizeof(struct in_addr)];
    
    flag = inet_pton(AF_INET, IPv4, buf);
    if(flag != 1){
        if(flag == 0)
            printf("Error found: IP inserted is not valid\n");
        if(flag == -1)
            printf("Error found: %s\n", strerror(errno));
        exit(1);
    }
    return 0;
}

int validar_port(*char port){

    int port_number = atoi(port);

    if(port_number>65536 || port_number<1){
        printf("Error found: port number inserted is not valid\n");
        exit(1);
    }

    return 0; 
}