#ifndef VALIDATION_H_INCLUDED
#define VALIDATION_H_INCLUDED

//Headers do ficheiro validation.c
void instructions();
int validar_IPv4 (char* IPv4);
int validar_port(char* port);
void validate_start(int argc, char* argv[]);
void user_interface();

#endif
