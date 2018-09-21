/*
 * Planificador.h
 *
 *  Created on: 20 sep. 2018
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

	/*
	 * Prototipos
	 */
	void *planificador_main(void *argumento);
	void *planificador_handler(fd_set *pool, void *argumento);
	void *planificador_nueva_conexion_handler(int iSocket, void *argumento);
	void agregar_todos_los_sockets_al_pool_planificador(PlanificadorNivelRelacion *planificador_nivel_relacion);
	int soy_el_nivel_en_planificador(int socket_nivel,fd_set *pool);
	PersonajeEnJuego *soy_un_personaje(fd_set *pool, t_list *personajes_en_juego_lista_internal);
	void remover_personaje_del_planificador(PersonajeEnJuego *remover,t_list *personajes_en_juego_lista_internal);

	/*****************Funciones del utilizadas por el dispatcher*****************************/
	void agregar_en_la_cola_de_ready(PersonajeEnJuego *personaje_en_juego, t_queue *ready,sem_t *semaforo_id, sem_t *mutex_id);
	void remover_de_la_lista_de_ready(PersonajeEnJuego *personaje_en_juego, t_queue *ready, sem_t *semaforo_id, sem_t *mutex_id);
	PersonajeEnJuego *tomar_uno_de_la_cola_de_ready(t_queue *ready, sem_t *semaforo_id, sem_t *mutex_id);
	void imprimir_lista(char *title, char *nombre_nivel, t_list *lista);
	void remover_de_la_lista_de_bloqueados(PersonajeEnJuego *personaje_en_juego, t_list *bloqueados);
	void agregar_en_la_lista_de_bloqueados(PersonajeEnJuego *personaje_en_juego, t_list *bloqueados);
	PersonajeEnJuego *tomar_de_la_lista_de_bloqueados(char recurso, t_list *bloqueados);

	void *dispatcher_handler(void *argumento);


	/*****************procedimientos para manejar cada mensaje*****************/
	/*de nivel*/
	void recibi_quien_sos(int iSocket);
	void nivel_listo(PlanificadorNivelRelacion *planificador_nivel_relacion);
	/*de personaje*/
	void recurso_asignado_sincronizado(PersonajeEnJuego *personaje_en_juego,PlanificadorNivelRelacion *planificador_nivel_relacion);
	void recibi_personaje_ingresando(int iSocket,char *mensaje, PlanificadorNivelRelacion *planificador_nivel_relacion);
	void turno_concluido(PlanificadorNivelRelacion *planificador_nivel_relacion);
	void turno_concluido_con_obtencion_de_recurso(PlanificadorNivelRelacion *planificador_nivel_relacion, char *mensaje);
	void turno_concluido_por_quedar_bloqueado(PlanificadorNivelRelacion *planificador_nivel_relacion, char *mensaje);
	void finalizo_el_nivel(int socket_personaje,PlanificadorNivelRelacion *planificador_nivel_relacion);
	void gestionar_desconexion_de_un_personaje(PersonajeEnJuego *personaje_en_juego, PlanificadorNivelRelacion *planificador_nivel_relacion, Estado estado);
	void enviar_movimiento_permitido_con_espera(PlanificadorNivelRelacion *planificador_nivel_relacion);
#endif

