
#ifndef CONSOLA_H_
#define CONSOLA_H_

typedef struct{
	char* pathMetadata;
	char* ip;
	int puerto;
}t_consola;
/********lo de clari**/

typedef int Function();
Function* func;
typedef void VFunction();
typedef char *CPFunction();
typedef char **CPPFunction();

CPPFunction* rl_arrempted_completion_function;
//char * rl_readline_name;
int quiere_salir_consola;

typedef struct{
	char* name;
	Function* func;
	char* doc;

}COMMAND;

char *command_generator ();
char **fileman_completion ();
char *stripwhite ();
COMMAND *find_command ();
char ** completion_matches (char *text, CPFunction *entry_func);
int comando_finalizar(), comando_ejecutar(), comando_metricas(), comando_status();

void ejecutar_consola_con_historial(char*);
void initialize_readline(void);


/********************/

#endif /* CONSOLA_H_ */
