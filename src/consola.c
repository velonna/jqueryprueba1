#include "Consola.h"

#define MAXDATASIZE = 100

t_consola consola;
t_config* consolaConfig;

void mandarMensaje(){
	struct sockaddr_storage their_add;
	socklen_t addr_size;
	struct addrinfo hints, *res;
	int sockfd, new_fd, status;
	printf("Hola, soy mandarMensaje\n");

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((status = getaddrinfo(NULL, consola.puerto, &hints, &res)) != 0){
		printf("Error en getaddrinfo");
	}

	if ((status = sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) != 0){
		printf("Error creando socket servidor");
	}
	if ((status = bind(sockfd, res->ai_addr, res->ai_addrlen)) != 0){ // probablemente innecesario para solamente mandar un mensaje
		printf("Error bindeando");
	}
	if ((status == connect(sockfd, res->ai_addr, res->ai_addrlen)) == -1){
		printf("Error al conectar");
		close(sockfd);
	}
	char *message= "Hola";
	int length, bytes_sent;
	length = strlen(message);
	bytes_sent = send(sockfd, message, length, 0);
	if (status != -1){
		close(sockfd);
	}
	exit(1);

}

int main(int argc, char *argv[]) {

	printf("Hola mundo \n");
	if(!setearConsola()){
		printf("Error seteando consola");
	}
	printf("Puerto: %d \n", consola.puerto);
	printf("IP: %s \n", consola.ip);

	pthread_t thread_Sockets;
	if (pthread_create(thread_Sockets, NULL, (void*) mandarMensaje, NULL) != 0) {
		printf("Error al crear thread_Sockets");
		return 0;
	}else{
		printf("Thread creado \n");
	}

	pthread_join(thread_Sockets, NULL);

	printf("Me voy");

	return EXIT_SUCCESS;
}

int setearConsola(){
	// en teor�a tendr�a que pasar la ruta de la carpeta para buscar el archivo de configuraci�n
	// le hardcodee la direcci�n para que sea m�s f�cil testear
	consola.pathConfig = malloc(strlen("/home/utnso/workspace/tp-2018-c-SO-Grupo/") + 8);
	strcpy(consola.pathConfig, "/home/utnso/workspace/tp-2018-2c-SO-Grupo");
	strcat(consola.pathConfig, "/config");
	consolaConfig = config_create(consola.pathConfig);
	char* ip = config_get_string_value(consolaConfig, "IP_KERNEL");
	consola.ip = malloc(strlen(ip) + 1);
	strcpy(consola.ip, ip);
	consola.puerto = config_get_int_value(consolaConfig, "PUERTO_KERNEL");
	printf("Consola seteada \n");

	return(1);
}
/**************lo de clari*****************
 *
 * #include "consola-safa.h"

COMMAND commands[] = {
		{"ejecutar", comando_ejecutar, "Pasando como parametro un path del script, permitira el ingreso de un nuevo programa"},
		{"status", comando_status, "Muestra el estado de las colas"},
		{"finalizar", comando_finalizar, "Pasa el proceso a la cola de EXIT"},
		{"metricas", comando_metricas, "Detalla metricas correspondientes a un DTB correspondiente"}
};


int comando_finalizar(arg)
char* arg;{
	return 0;
}
int comando_ejecutar(arg)
char* arg;{
	return 0;

}
int comando_metricas(arg)
char* arg;{
	return 0;

}
int comando_status(arg)
char* arg;{
	return 0;

}

void ejecutar_consola_con_historial(char* prompt){
	char* line, *s;
	initialize_readline();
	quiere_salir_consola=0;
	while(!quiere_salir_consola){
		line = readline(prompt);
		if(line){
			s = stripwhite(line);
			if(*s){
				add_history(s);
				execute_line(s);

			}
			free(line);
		}
	}
}
char** fileman_completion(text,start,end) char*text;//que  estoy mandando??
int start, end;{
	char** matches;
	matches = (char**)NULL;
	if(start == 0) matches = completion_matches(text, command_generator);
	return matches;
}
char *
dupstr (s)
     int s;
{
  char *r;

  r = xmalloc (strlen (s) + 1);
  strcpy (r, s);
  return (r);
}

char* command_generator(text, state)
char *text;
int state;{
	static int list_index, len;
	char *name;
	if(!state){
		list_index = 0;
		len = strlen(text);
	}
	while(name = commands[list_index].name){
		list_index++;
		if(strncmp(name, text, len)==0)
			return (dupstr(name));
	}
	return ((char*)NULL);
}
void initialize_readline(void){
	rl_readline_name = "Consola SAFA";
	rl_arrempted_completion_function = (CPPFunction *)fileman_completion;
}

COMMAND *
find_command (name)
     char *name;
{
  register int i;

  for (i = 0; commands[i].name; i++)
    if (strcmp (name, commands[i].name) == 0)
      return (&commands[i]);

  return ((COMMAND *)NULL);
}

char *
stripwhite (string)
     char *string;
{
  register char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}

/* Execute a command line. */
int
execute_line (line)
     char *line;
{
  register int i;
  COMMAND *command;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while (line[i] && whitespace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !whitespace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  command = find_command (word);

  if (!command)
    {
      fprintf (stderr, "%s: No such command for FileMan.\n", word);
      return (-1);
    }

  /* Get argument to command, if any. */
  while (whitespace (line[i]))
    i++;

  word = line + i;

  /* Call the function. */
  return ((*(command->func)) (word));
}

 */















