/*
 * Logueador.h
 *
 *  Created on: 20 sep. 2018
 *      Author: utnso
 */

#ifndef LOGUEADOR_H_
#define LOGUEADOR_H_
// Definimos algunas constantes para nuestro código
#define IP "127.0.0.1"
#define PUERTO "8080"

// Definimos algunas variables globales
t_log * logger;

// A continuacion estan las estructuras con las que nos vamos a manejar.
typedef struct  {
  int id_mensaje;
  int legajo;
  char nombre[40];
  char apellido [40];
} __attribute__((packed)) Alumno;

typedef struct {
  int id;
  int len;
} __attribute__((packed)) ContentHeader;

// Finalmente, los prototipos de las funciones que vamos a implementar
void configure_logger();
int  connect_to_server(char * ip, char * port);
void wait_hello(int socket);
Alumno read_hello();
void send_hello(int socket, Alumno alumno);
void * wait_content(int socket);
void send_md5(int socket, void * content);
void wait_confirmation(int socket);
void exit_gracefully(int return_nr);
void _exit_with_error(int socket, char* error_msg, void * buffer);
#endif /* LOGUEADOR_H_ */
