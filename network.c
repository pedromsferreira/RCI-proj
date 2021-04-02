#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "validation.h"

#define BUF_SIZE 1024

//global variables
int sockfd;
struct addrinfo hints, *server_info;

//default IP e UDP port
char defaultIP[16] = "193.136.138.142";
char defaultUDP[6] = "59000"; 

//Initialize socket for UDP connection
void UDP_socket(int argc, char* IP, char* UDP){
    //Variables
    int flag;


    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd == -1){
        printf("Error: %s\n", strerror(errno));
        exit(1);
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    if(argc == 3)
        flag = getaddrinfo(defaultIP,defaultUDP,&hints,&server_info);
    else
        flag = getaddrinfo(IP,UDP,&hints,&server_info);

    if(flag != 0){
        printf("Error: %s\n", gai_strerror(flag));
        exit(1);
    }
    return;
}

void state_machine(int argc, char* argv[]){
    //Variables
    int flag = 0, fd_ready, maxfd;
    enum {reg, notreg, exiting} state;
    fd_set read_fd;


    //iniciar conexão ao servidor
    //preparar o socket UDP
    UDP_socket(argc,argv[3],argv[4]);

    //entrar na máquina de estados como not registered
    state = notreg;
    printf("User> ");
    fflush(stdout);

    while(state!=exiting)
    {
        //initialize fd
        FD_ZERO(&read_fd);
        //initialize states
        switch(state)
        {
            case notreg:
                FD_SET(0,&read_fd);
                maxfd = 0;
                break;
            
            
        }

        //await for fds ready to be read
        fd_ready = select(maxfd+1,&read_fd,(fd_set*) NULL,(fd_set*) NULL,(struct timeval*) NULL);
        //in case return value is wrong
        if(fd_ready <= 0){
            printf("Error during select: %s\n", strerror(errno));
            //exit strat goes here
        }

        for(;fd_ready;fd_ready-=1)
        {
            switch(state)
            {
                //for when program is not registered in the network
                case notreg:
                    //if stdin (fd = 0) interreptud select
                    if(FD_ISSET(0,&read_fd))
                    {
                        FD_CLR(0,&read_fd);
                    
                        flag = user_interface();
                    }
            }
        }


    }



    return;
}


//o que tinhamos no main

//Variables
//int flag = 0, bootTCP;
//char command1[7], command2[128], net[128], id[128], bootIP[256];

//interface do utilizador 
    /*
    while(1){
        printf("Por favor introduza um comando\n"); 
        flag = sscanf("%s %s %s %s %d", command1, command2, id, bootIP, &bootTCP);
        
        if(strcmp(command1,"join")){ 
            if(flag==3){

            }
            
            else if(flag==5){
                
            }

            else{
                printf("Deu shit, go back");
            }

        }
        else if(strcmp(command1,"create") && flag == 2){
            
        }
        else if(strcmp(command1,"get") && flag == 2){
            
        }
        else if((strcmp(command1,"show") && flag == 2) || (((strcmp(command1,"st") || strcmp(command1,"sr") || strcmp(command1,"sc")) && (flag == 1)))){
            
            if(strcmp(command2, "topology") || strcmp(command1,"st")){

            }
            
            else if(strcmp(command2, "routing") || strcmp(command1,"sr")){
                
            }
            
            else if(strcmp(command2, "cache") || strcmp(command1,"sc")){

            }
        }
        else if(strcmp(command1,"leave") && flag == 1){

        }
        else if(strcmp(command1,"exit") && flag == 1){

        }
        //se não for encontrado qualquer comando da lista
        else{
            printf("Deu shit, go back");
        }
    }
    */