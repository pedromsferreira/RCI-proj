#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "validation.h"

int main(int argc, char* argv[]){
    //Variables
    int flag = 0, bootTCP;
    char command1[7], command2[128], net[128], id[128], bootIP[256];

    //code
    validate_start(argc, argv);

    //conectar nó


    //interface do utilizador 
    while(1){
        printf("Por favor introduza um comando\n"); //Maybe fazer lista de comandos
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
        else if((strcmp(command1,"show") && flag == 2) || (strcmp(command1,"st") || strcmp(command1,"sr") || strcmp(command1,"sc") && flag == 1)){
            
            if(strcmp(command2, "topology" || strcmp(command1,"st")){

            }
            
            else if(strcmp(command2, "routing" || strcmp(command1,"sr")){
                
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
    
    
    return 0;
}

