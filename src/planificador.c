#include "Planificador.h"

/*
 * Funcion principal, se llama cuando se crea el hilo
 */
void *planificador_main(void *argumento)
{
	debug("INICIO planificador_main");
	PlanificadorNivelRelacion *planificador_nivel_relacion=argumento;
	info("Hilo planificador para %s iniciado",planificador_nivel_relacion->nivel_nombre);

	/*Conecto el planificador al nivel*/
	info("El planificador del nivel %s establece conexion con el nivel en la IP:%s puerto:%d",planificador_nivel_relacion->nivel_nombre,planificador_nivel_relacion->nivel->ip,planificador_nivel_relacion->nivel->puerto);
	conectar_a(&planificador_nivel_relacion->socket_nivel,planificador_nivel_relacion->nivel);

	/*Pongo a escuchar al planificador*/
	info("El planificador del nivel %s comienza a escuchar personajes",planificador_nivel_relacion->nivel_nombre);
	escuchar_en(&planificador_nivel_relacion->socket_planificador,planificador_nivel_relacion->planificador->puerto);


	while(VERDADERO)
	{
		inicializar_pool(&planificador_nivel_relacion->pool_planificador);
		agregar_todos_los_sockets_al_pool_planificador(planificador_nivel_relacion);
		checkear_pool_sockets_para_conexiones_nuevas(planificador_nivel_relacion->socket_planificador, &planificador_nivel_relacion->pool_planificador,planificador_nueva_conexion_handler,planificador_nivel_relacion,planificador_handler,planificador_nivel_relacion);
	}
	debug("FINALIZO planificador_main");
	return (void*)BIEN;
}

/*
 * Funcion que se pasa como puntero para manejar los ingresos nuevos
 * al socket de escucha del planificador
 */
void *planificador_nueva_conexion_handler(int iSocket, void *argumento){
	debug("INICIO planificador_nueva_conexion_handler");
	PlanificadorNivelRelacion *planificador_nivel_relacion=argumento;
	info("Se detecto un ingreso");
	enviar(iSocket,QUIEN_SOS);
	list_add(planificador_nivel_relacion->sockets_anonimos_lista,(void *)iSocket);
	debug("FINALIZO planificador_nueva_conexion_handler");
	return (void*)BIEN;
}

/*
 * En esta funcion ingresa cada vez que alguien conectado al planificador
 * envia algun mensaje
 */
void *planificador_handler(fd_set *pool, void *argumento){
	debug("INICIO planificador_handler");
	PlanificadorNivelRelacion *planificador_nivel_relacion=argumento;
	PersonajeEnJuego *personaje_en_juego;
	int iSocket=(int)NULL;
	char *mensaje;
	char *mensaje2;
	int tipo_mensaje=0;

	if((iSocket=soy_un_anonimo(pool, planificador_nivel_relacion->sockets_anonimos_lista))!=(int)NULL)
	{
		track("entro a anonimos");
		switch(tipo_mensaje=recibir_con_dos_mensaje(iSocket,&mensaje,&mensaje2))
		{
		case PERSONAJE_INGRESANDO_AL_PLANIFICADOR:
			remover_anonimo(iSocket, planificador_nivel_relacion->sockets_anonimos_lista);
			recibi_personaje_ingresando(iSocket, mensaje, planificador_nivel_relacion);
			break;
		case SOCKET_DESCONECTADO:
			remover_anonimo(iSocket, planificador_nivel_relacion->sockets_anonimos_lista);
			FD_CLR(iSocket,pool);
			close(iSocket);
			break;
		default:
			warning("El mensaje no corresponde para este contexto ingreso en anonimo");
			break;
		}
	}else if((personaje_en_juego=soy_un_personaje(pool,planificador_nivel_relacion->personajes_en_juego_lista))!=NULL)
	{
		track("entro a personajes");
		iSocket=personaje_en_juego->socket_actual;
		switch(tipo_mensaje=recibir_con_mensaje(iSocket,&mensaje))
		{
		case RECURSO_ASIGNADO_POR_DESBLOQUEO_OK:
			recurso_asignado_sincronizado(personaje_en_juego,planificador_nivel_relacion);
			break;
		case TURNO_CONCLUIDO:
			turno_concluido(planificador_nivel_relacion);
			break;
		case TURNO_CONCLUIDO_POR_BLOQUEO:
			turno_concluido_por_quedar_bloqueado(planificador_nivel_relacion, mensaje);
			break;
		case TURNO_CONCLUIDO_POR_RECURSO_OBTENIDO:
			turno_concluido_con_obtencion_de_recurso(planificador_nivel_relacion, mensaje);
			break;
		case TURNO_CONCLUIDO_NIVEL_FINALIZADO:
			finalizo_el_nivel(iSocket,planificador_nivel_relacion);
			break;
		case VOY_A_MORIR:
			gestionar_desconexion_de_un_personaje(personaje_en_juego, planificador_nivel_relacion, MUERTO);
			enviar(personaje_en_juego->socket_actual, MUERTE_OK);
			break;
		case SOCKET_DESCONECTADO:
			gestionar_desconexion_de_un_personaje(personaje_en_juego, planificador_nivel_relacion, DESCONECTADO);
			personaje_en_juego->socket_actual=-1;
			FD_CLR(iSocket,pool);
			close(iSocket);
			break;
		default:
			warning("El mensaje no corresponde para este contexto ingreso en personaje");
			break;
		}
	}else if((iSocket=soy_el_nivel_en_planificador(planificador_nivel_relacion->socket_nivel,&planificador_nivel_relacion->pool_planificador))!=(int)NULL)
	{
		track("PLANIFICADOR: entro a nivel");
		switch(tipo_mensaje=recibir_con_mensaje(iSocket,&mensaje))
		{
		case QUIEN_SOS:
			recibi_quien_sos(iSocket);
			break;
		case NIVEL_LISTO:
			nivel_listo(planificador_nivel_relacion);
			break;
		case SOCKET_DESCONECTADO:
			info("PLANIFICADOR %s APAGANDO: Se desconecto el nivel",planificador_nivel_relacion->nivel_nombre);
			FD_CLR(iSocket,pool);
			pthread_exit(NULL);
			break;
		default:
			warning("El mensaje no corresponde para este contexto ingreso en anonimo");
			break;
		}
	}else
	{
		error("El socket no esta en ninguna lista, no se puede procesar");
	}
	debug("FINALIZO planificador_handler");
	return (void*)BIEN;
}

/*
 * Agrega al pool del planificador los sockets que necesita chequear para este caso en el select
 */
void agregar_todos_los_sockets_al_pool_planificador(PlanificadorNivelRelacion *planificador_nivel_relacion)
{
	debug("INICIO agregar_todos_los_sockets_al_pool_planificador");
	agregar_socket_al_pool(planificador_nivel_relacion->socket_planificador,&planificador_nivel_relacion->pool_planificador);
	agregar_socket_al_pool(planificador_nivel_relacion->socket_nivel,&planificador_nivel_relacion->pool_planificador);

	int ii=0;
	track("Anonimos %d",planificador_nivel_relacion->sockets_anonimos_lista->elements_count);
	for(ii=0;ii<planificador_nivel_relacion->sockets_anonimos_lista->elements_count;ii++ )
	{
		agregar_socket_al_pool((int)list_get(planificador_nivel_relacion->sockets_anonimos_lista,ii), &planificador_nivel_relacion->pool_planificador);
	}

	track("Personajes %d",planificador_nivel_relacion->personajes_en_juego_lista->elements_count);
	for(ii=0;ii<planificador_nivel_relacion->personajes_en_juego_lista->elements_count;ii++ )
	{
		PersonajeEnJuego * item=list_get(planificador_nivel_relacion->personajes_en_juego_lista,ii);
		agregar_socket_al_pool(item->socket_actual, &planificador_nivel_relacion->pool_planificador);
	}
	debug("FINALIZO agregar_todos_los_sockets_al_pool_planificador");
}


/*
 * Verifica si un personaje de la lista parametro esta intentando comunicarse en el pool parametro
 */
PersonajeEnJuego *soy_un_personaje(fd_set *pool, t_list *personajes_en_juego_lista_internal){
	debug("INICIO soy_un_personaje");
	/*busco si es un personaje*/
	int personaje_atencion_comparer(PersonajeEnJuego *item) {
		return es_este_socket_llamando(item->socket_actual, pool);
	}
	debug("FINALIZO soy_un_personaje");
	return (PersonajeEnJuego *)list_find(personajes_en_juego_lista_internal,(void *)personaje_atencion_comparer);
}

/*
 * Remueve un personaje de la lista parametro, pero no destruye el item, solo lo quita de la lista
 */
void remover_personaje_del_planificador(PersonajeEnJuego *remover, t_list *personajes_en_juego_lista_internal){
	debug("INICIO remover_personaje_del_planificador");
	/*busco el socket en la lista de personajes*/
	int personaje_por_socket_comparer(PersonajeEnJuego *item) {
		return string_equals_ignore_case(item->personaje_nombre,remover->personaje_nombre);
	}

	list_remove_by_condition(personajes_en_juego_lista_internal,(void *)personaje_por_socket_comparer);
	debug("FINALIZO remover_personaje_del_planificador");
}

/*
 * Cuando se detecta movimiento en el pool parametro esta funcion evalua si se tratata de
 * un socket que se identifica con un nivel conectado y, si es asi, lo retorna, sino retorna NULL
 */
int soy_el_nivel_en_planificador(int socket_nivel,fd_set *pool){
	debug("INICIO soy_el_nivel_en_planificador");
	if(es_este_socket_llamando(socket_nivel,pool))
	{
		debug("FINALIZO soy_el_nivel_en_planificador");
		return socket_nivel;
	}else
	{
		debug("FINALIZO soy_el_nivel_en_planificador");
		return (int)NULL;
	}
}

/*De nivel*/
void recibi_quien_sos(int iSocket)
{
	debug("INICIO recibi_quien_sos");
	enviar(iSocket,PLANIFICADOR);
	info("Le informo al nivel que soy su planificador asignado");
	debug("FINALIZO recibi_quien_sos");
}

void nivel_listo(PlanificadorNivelRelacion *planificador_nivel_relacion){
	debug("INICIO nivel_listo");
	track("Creando hilo despachador del planificador del nivel %s",planificador_nivel_relacion->nivel_nombre);
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&planificador_nivel_relacion->dispatcher_thread, &attr, dispatcher_handler, (void *)planificador_nivel_relacion);
	info("Nivel %s listo. Este planificador comienza a planificar",planificador_nivel_relacion->nivel_nombre);
	debug("FINALIZO nivel_listo");
}

/*De personaje*/

void recurso_asignado_sincronizado(PersonajeEnJuego *personaje_en_juego,PlanificadorNivelRelacion *planificador_nivel_relacion){
	debug("INICIO recurso_asignado_sincronizado");
	personaje_en_juego->recurso_que_espera=' ';
	agregar_en_la_cola_de_ready(personaje_en_juego, planificador_nivel_relacion->ready,planificador_nivel_relacion->ready_semaforo_id,planificador_nivel_relacion->ready_mutex_id);
	info("Personaje '%s' recibio recurso para desbloquearse", personaje_en_juego->personaje_nombre);
	debug("FINALIZO recurso_asignado_sincronizado");
}

void recibi_personaje_ingresando(int iSocket,char *mensaje, PlanificadorNivelRelacion *planificador_nivel_relacion)
{
	debug("INICIO recibi_personaje_ingresando");
	char *personaje_nombre=string_duplicate(mensaje);
	PersonajeEnJuego *personaje_en_juego= buscar_un_personaje_por_nombre(personaje_nombre);
	if(personaje_en_juego!=NULL)
	{
		personaje_en_juego->socket_actual=iSocket;
		list_add(planificador_nivel_relacion->personajes_en_juego_lista,personaje_en_juego);
		agregar_en_la_cola_de_ready(personaje_en_juego,planificador_nivel_relacion->ready,planificador_nivel_relacion->ready_semaforo_id,planificador_nivel_relacion->ready_mutex_id);

		info("El personaje '%s' esta en la cola de ready",personaje_nombre);
	}else
	{
		error("El personaje no existe en el orquestador, no se acepta su conexion");
		close(iSocket);
	}
	debug("FINALIZO recibi_personaje_ingresando");
}

void turno_concluido_por_quedar_bloqueado(PlanificadorNivelRelacion *planificador_nivel_relacion, char *mensaje){
	debug("INICIO turno_concluido_por_quedar_bloqueado");
	char *recurso=mensaje;
	agregar_en_la_lista_de_bloqueados(planificador_nivel_relacion->running,planificador_nivel_relacion->blocked);
	planificador_nivel_relacion->running->recurso_que_espera=mensaje[0];
	info("El personaje '%s' se bloquea hasta que se libere el recurso '%s' y se envio a la lista de bloqueados",planificador_nivel_relacion->running->personaje_nombre, recurso);
	planificador_nivel_relacion->running=NULL;
	signal_semaforo(planificador_nivel_relacion->running_mutex_id);
	signal_semaforo(planificador_nivel_relacion->interruption_mutex_id);
	debug("FINALIZO turno_concluido_por_quedar_bloqueado");
}

void finalizo_el_nivel(int socket_personaje,PlanificadorNivelRelacion *planificador_nivel_relacion){
	debug("INICIO finalizo_el_nivel");

	char *personaje_nombre=planificador_nivel_relacion->running->personaje_nombre;
	char *nivel_nombre=planificador_nivel_relacion->nivel_nombre;

	remover_personaje_del_planificador(planificador_nivel_relacion->running,planificador_nivel_relacion->personajes_en_juego_lista);
	planificador_nivel_relacion->running->socket_actual=-1;
	planificador_nivel_relacion->running=NULL;

	info("El personaje '%s' finalizo el nivel '%s'",personaje_nombre, nivel_nombre);

	enviar(socket_personaje,NIVEL_FINALIZADO_CON_EXITO);

	signal_semaforo(planificador_nivel_relacion->running_mutex_id);
	signal_semaforo(planificador_nivel_relacion->interruption_mutex_id);
	debug("FINALIZO finalizo_el_nivel");
}

void turno_concluido_con_obtencion_de_recurso(PlanificadorNivelRelacion *planificador_nivel_relacion, char *mensaje){
	debug("INICIO turno_concluido_con_obtencion_de_recurso");
	char *recurso=mensaje;
	agregar_en_la_cola_de_ready(planificador_nivel_relacion->running,planificador_nivel_relacion->ready,planificador_nivel_relacion->ready_semaforo_id,planificador_nivel_relacion->ready_mutex_id);
	info("El personaje '%s' obtuvo el recurso '%s' y se envio a la cola de ready",planificador_nivel_relacion->running->personaje_nombre, recurso);
	planificador_nivel_relacion->running=NULL;
	signal_semaforo(planificador_nivel_relacion->running_mutex_id);
	signal_semaforo(planificador_nivel_relacion->interruption_mutex_id);
	debug("FINALIZO turno_concluido_con_obtencion_de_recurso");
}

void turno_concluido(PlanificadorNivelRelacion *planificador_nivel_relacion){
	debug("INICIO turno_concluido");
	planificador_nivel_relacion->quantum_consumido++;
	signal_semaforo(planificador_nivel_relacion->running_mutex_id);
	signal_semaforo(planificador_nivel_relacion->interruption_mutex_id);
	debug("FINALIZO turno_concluido");
}



void gestionar_desconexion_de_un_personaje(PersonajeEnJuego *personaje_en_juego, PlanificadorNivelRelacion *planificador_nivel_relacion, Estado estado){
	debug("INICIO gestionar_desconexion_de_un_personaje");

	personaje_en_juego->estado=estado;
	remover_de_la_lista_de_ready(personaje_en_juego,planificador_nivel_relacion->ready,planificador_nivel_relacion->ready_semaforo_id,planificador_nivel_relacion->ready_mutex_id);
	remover_de_la_lista_de_bloqueados(personaje_en_juego,planificador_nivel_relacion->blocked);
	remover_personaje_del_planificador(personaje_en_juego,planificador_nivel_relacion->personajes_en_juego_lista);
	if(planificador_nivel_relacion->running==personaje_en_juego){
		//Si es el personaje que se estaba ejecutando, habilito que ejecute uno nuevo
		planificador_nivel_relacion->running=NULL;
		signal_semaforo(planificador_nivel_relacion->running_mutex_id);
		signal_semaforo(planificador_nivel_relacion->interruption_mutex_id);
	}
	info("El personaje %s se ha desconectado", personaje_en_juego->personaje_nombre);
	debug("FINALIZO gestionar_desconexion_de_un_personaje");
}

/****************************************************************************************************/
/************************************Dispatcher******************************************************/

/*
 * Esta funcion se pasa como puntero cuando se crea el hilo del dispatcher al momento que el nivel me confirma
 * que esta listo para comenzar a trabajar. El parametro es la estructura correspondiente a este planificador
 * de donde se obtendran todas las colas y listas necesarias para manejar el dispatcher
 */
void *dispatcher_handler(void *argumento)
{
	debug("INICIO dispatcher_handler");
	PlanificadorNivelRelacion *planificador_nivel_relacion=argumento;
	track("Dispatcher '%s' iniciado",planificador_nivel_relacion->nivel_nombre);
	while(VERDADERO)
	{
		// Este tiempo se utiliza para que se puedan apreciar los movimientos de los personajes y no vaya a velocidad de CPU real

		sleep(plataforma_config->tiempo_entre_ejecuciones / 1000);
		usleep((plataforma_config->tiempo_entre_ejecuciones % 1000)*1000);

		debug("DISPATCHER - Planificador '%s' espera autorizacion para tomar uno de la cola de ready",planificador_nivel_relacion->nivel_nombre);
		wait_semaforo(planificador_nivel_relacion->running_mutex_id);
		debug("DISPATCHER - Planificador '%s' autorizado a tomar uno de la cola de ready",planificador_nivel_relacion->nivel_nombre);

		// es menor o igual porque el quantum me lo pueden modificar de afuera, y hay uno ejecutando
		if(plataforma_config->quantum<=planificador_nivel_relacion->quantum_consumido
				&& planificador_nivel_relacion->running!=NULL){
			agregar_en_la_cola_de_ready(planificador_nivel_relacion->running,planificador_nivel_relacion->ready,planificador_nivel_relacion->ready_semaforo_id,planificador_nivel_relacion->ready_mutex_id);
			info("Se termino el quantum para el personaje '%s' y se envio a la cola de ready",planificador_nivel_relacion->running->personaje_nombre);
			planificador_nivel_relacion->running=NULL;
		}

		imprimir_lista("Ready: ",planificador_nivel_relacion->nivel_nombre, planificador_nivel_relacion->ready->elements);
		imprimir_lista("Blocked: ",planificador_nivel_relacion->nivel_nombre,  planificador_nivel_relacion->blocked);
		if(planificador_nivel_relacion->running==NULL){//Si no hay ninguno ejecutando
			planificador_nivel_relacion->running = tomar_uno_de_la_cola_de_ready(planificador_nivel_relacion->ready,planificador_nivel_relacion->ready_semaforo_id, planificador_nivel_relacion->ready_mutex_id);
			planificador_nivel_relacion->quantum_consumido=0;
		}
		info("%s-Running: %s",planificador_nivel_relacion->nivel_nombre, planificador_nivel_relacion->running->personaje_nombre);

		wait_semaforo(planificador_nivel_relacion->interruption_mutex_id);
		debug("DISPATCHER - Planificador '%s' tomo uno de la cola de ready",planificador_nivel_relacion->nivel_nombre);
		enviar_movimiento_permitido_con_espera(planificador_nivel_relacion);
		debug("DISPATCHER - Planificador '%s' envio mensaje de movimiento permitido",planificador_nivel_relacion->nivel_nombre);
	}
	debug("FINALIZO dispatcher_handler");
	return (void*)BIEN;
}

void enviar_movimiento_permitido_con_espera(PlanificadorNivelRelacion *planificador_nivel_relacion){
	debug("INICIO enviar_movimiento_permitido_con_espera");

	if(planificador_nivel_relacion->running==NULL)
		debug("personaje es null*****************************************");
	debug("%s",planificador_nivel_relacion->running->personaje_nombre);
	debug("%d",planificador_nivel_relacion->quantum_consumido);
	debug("%d",plataforma_config->quantum);

	info("Se autoriza el movimiento a '%s' para Q=%d/%d",
			planificador_nivel_relacion->running->personaje_nombre,
			planificador_nivel_relacion->quantum_consumido,
			plataforma_config->quantum);

	enviar(planificador_nivel_relacion->running->socket_actual,MOVIMIENTO_PERMITIDO);

	debug("FINALIZO enviar_movimiento_permitido_con_espera");
}

/*
 * Quita un personaje en juego de la lista de ready, esta funcion solo lo quita, no lo destruye.
 * Se utiliza para quitar uno que se desconecto y no es el primero,
 * sino que es uno que esta por al mitad de la cola
 */
void remover_de_la_lista_de_ready(PersonajeEnJuego *personaje_en_juego, t_queue *ready, sem_t *semaforo_id, sem_t *mutex_id){
	debug("INICIO remover_de_la_lista_de_ready");
	int personaje_por_nombre_comparer(PersonajeEnJuego *item) {
		return item->personaje_nombre ==personaje_en_juego->personaje_nombre;
	}
	wait_semaforo(mutex_id);
	if(list_remove_by_condition(ready->elements,(void *)personaje_por_nombre_comparer)!=NULL){
		wait_semaforo(semaforo_id);
	}

	signal_semaforo(mutex_id);
	debug("FINALIZO remover_de_la_lista_de_ready");
}

/*
 * Quita un personaje en juego de la lista de bloqueados, esta funcion solo lo quita, no lo destruye.
 */
void remover_de_la_lista_de_bloqueados(PersonajeEnJuego *personaje_en_juego, t_list *bloqueados){
	debug("INICIO remover_de_la_lista_de_bloqueados");
	int personaje_por_nombre_comparer(PersonajeEnJuego *item) {
		return item->personaje_nombre ==personaje_en_juego->personaje_nombre;
	}

	list_remove_by_condition(bloqueados,(void *)personaje_por_nombre_comparer);

	debug("FINALIZO remover_de_la_lista_de_bloqueados");
}

/*
 * Quita un personaje en juego de la lista de bloqueados, esta funcion solo lo quita, no lo destruye.
 */
PersonajeEnJuego *tomar_de_la_lista_de_bloqueados(char recurso, t_list *bloqueados){
	debug("INICIO tomar_de_la_lista_de_bloqueados");
	int personaje_por_recurso_comparer(PersonajeEnJuego *item) {
		return item->recurso_que_espera == recurso;
	}
	debug("FINALIZO tomar_de_la_lista_de_bloqueados");

	PersonajeEnJuego *personaje_en_juego=list_remove_by_condition(bloqueados,(void *)personaje_por_recurso_comparer);


	return personaje_en_juego;
}

/*
 * Toma el primer personaje en juego de la cola de ready y lo quita de la cola
 */
PersonajeEnJuego *tomar_uno_de_la_cola_de_ready(t_queue *ready, sem_t *semaforo_id, sem_t *mutex_id){
	debug("INICIO tomar_uno_de_la_cola_de_ready");
	PersonajeEnJuego *personaje_en_juego=NULL;
	wait_semaforo(semaforo_id);
	wait_semaforo(mutex_id);
	personaje_en_juego= queue_pop(ready);

	signal_semaforo(mutex_id);
	debug("FINALIZO tomar_uno_de_la_cola_de_ready");
	return personaje_en_juego;
}

/*
 * Agrego un personaje en juego a la cola de ready
 */
void agregar_en_la_cola_de_ready(PersonajeEnJuego *personaje_en_juego, t_queue *ready, sem_t *semaforo_id, sem_t *mutex_id){
	debug("INICIO agregar_en_la_cola_de_ready");
	wait_semaforo(mutex_id);
	personaje_en_juego->estado=LISTO;
	queue_push(ready,personaje_en_juego);

	signal_semaforo(semaforo_id);

	signal_semaforo(mutex_id);
	debug("FINALIZO agregar_en_la_cola_de_ready");
}

/*
 * Agrego un personaje en juego a la cola de bloqueados
 */
void agregar_en_la_lista_de_bloqueados(PersonajeEnJuego *personaje_en_juego, t_list *bloqueados){
	debug("INICIO agregar_en_la_lista_de_bloqueados");
	personaje_en_juego->estado=BLOQUEADO;
	list_add(bloqueados,personaje_en_juego);
	debug("FINALIZO agregar_en_la_lista_de_bloqueados");
}

/*Imprime una lista con un titulo*/
void imprimir_lista(char *title, char *nombre_nivel, t_list *lista){
	char *lista_impresa=string_new();
	t_link_element *element = lista->head;
	string_append(&lista_impresa,nombre_nivel);
	string_append(&lista_impresa,"-");
	string_append(&lista_impresa,title);
	while (element != NULL) {
		PersonajeEnJuego *personaje_en_juego=element->data;
		string_append(&lista_impresa,personaje_en_juego->personaje_nombre);
		element = element->next;
		if(element!=NULL)
			string_append(&lista_impresa,"<--");
	}
	info(lista_impresa);
}
