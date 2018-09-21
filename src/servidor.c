#include "s-afa.h"


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {//af_inet no es solo local
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int servidor(int puerto, int ip){

	    fd_set master;
	    fd_set read_fds;
	    int fdmax;

	    int listener;
	    int newfd;        // newly accept()ed socket descriptor
	    struct sockaddr_storage remoteaddr; // client address
	    socklen_t addrlen;

	    char buf[256];    // buffer for client data
	    int nbytes;

		char remoteIP[INET6_ADDRSTRLEN];

	    int yes=1;        // for setsockopt() SO_REUSEADDR, below
	    int i, j, rv;

		struct addrinfo hints, *ai, *p;

	    FD_ZERO(&master);    // clear the master and temp sets
	    FD_ZERO(&read_fds);

		// nos conectamos a un socket
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {//pasarle el puerto
			fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
			exit(1);
		}

		for(p = ai; p != NULL; p = p->ai_next) {
	    	listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (listener < 0) {
				continue;
			}

			// "direccion en uso" error message
			setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

			if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
				close(listener);
				continue;
			}

			break;
		}

		//si no conectamos
		if (p == NULL) {
			fprintf(stderr, "selectserver: failed to bind\n");
			exit(2);
		}

		freeaddrinfo(ai); // listoo

	    // listen
	    if (listen(listener, 10) == -1) {
	        perror("listen");
	        exit(3);
	    }

	    // add the listener to the master set
	    FD_SET(listener, &master);

	    // mantener el seguimiento de los descrpitor masgrandes
	    fdmax = listener; // mientras es este

	    // main loop
	    for(;;) {
	        read_fds = master; // copy it
	        puts("Safa escuchando conexiones");
	        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
	            perror("select");
	            exit(4);
	        }

	        //recorre por las instrucciones buscando datos para leer
	        for(i = 0; i <= fdmax; i++) {
	            if (FD_ISSET(i, &read_fds)) { // hay uno!!
	                if (i == listener) {
	                    // handle new connections
	                    addrlen = sizeof remoteaddr;
						newfd = accept(listener,
							(struct sockaddr *)&remoteaddr,
							&addrlen);

						if (newfd == -1) {
	                        perror("accept");
	                    } else {
	                        FD_SET(newfd, &master); // add to master set
	                        if (newfd > fdmax) {    // keep track of the max
	                            fdmax = newfd;
	                        }
	                        printf("selectserver: new connection from %s on "
	                            "socket %d\n",
								inet_ntop(remoteaddr.ss_family,
									get_in_addr((struct sockaddr*)&remoteaddr),
									remoteIP, INET6_ADDRSTRLEN),
								newfd);
	                    }
	                } else {
	                    // handle data from a client
	                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
	                        // got error or connection closed by client
	                        if (nbytes == 0) {
	                            // connection closed
	                            printf("selectserver: socket %d hung up\n", i);
	                        } else {
	                            perror("recv");
	                        }
	                        close(i); // bye!
	                        FD_CLR(i, &master); // remove from master set
	                    } else {
	                        // tenemos algunos datos de un cliente
	                        for(j = 0; j <= fdmax; j++) {
	                            // enviamos a todos!
	                            if (FD_ISSET(j, &master)) {
	                                // menos a nosotros y al oyente
	                                if (j != listener && j != i) {
	                                    if (send(j, buf, nbytes, 0) == -1) {
	                                        perror("send");
	                                    }
	                                }
	                            }
	                        }
	                    }
	                } // END handle data from client
	            } // END got new incoming connection
	        } // END looping through file descriptors
	    } // END for(;;)--and you thought it would never end!jamassss!

	    return 0;

}
/*

int  LeerArchivoDeConfiguracion(char *argv[]) {
	// Leer archivo de configuracion con las commons

	t_config* configuracion;
	configuracion = config_create(argv[1]);
	puertoEscucha = config_get_int_value(configuracion, "PUERTO");
	algoritmoPlanificacion = config_get_string_value(configuracion, "ALGORITMO");
	quantum = config_get_int_value(configuracion, "QUANTUM");
	gradoMultiprogramacion = config_get_int_value(configuracion, "MULTIPROGRAMACION");
	retardo = config_get_int_value(configuracion, "RETARDO_PLANIF");

	if (strcmp(algoritmoPlanificacion, "RR") == 0)
		algoritmo = RR;
	else {
		if (strcmp(algoritmoPlanificacion, "VRR") == 0)
			algoritmo = VRR;
		else {
			if (strcmp(algoritmoPlanificacion, "PROPIO") == 0)
				algoritmo = PROPIO;
			else {
				puts("Error al cargar el Algoritmo de planificación");
				exit(1);
			}
		}
	}

	return 1;
}*/

