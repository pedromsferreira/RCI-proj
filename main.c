#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "defines.h"
#include "validation.h"
#include "network.h"

int main(int argc, char* argv[]){
    struct sigaction act;
    
    memset(&act,0,sizeof act);
    act.sa_handler=SIG_IGN;
    if(sigaction(SIGPIPE,&act,NULL)==-1)
        exit(1);

    srand(time(0));
    //Variables

    //validação dos argumentos da consola
    validate_start(argc, argv);

    //entrar no select
    state_machine(argc, argv);


    return 0;
}

