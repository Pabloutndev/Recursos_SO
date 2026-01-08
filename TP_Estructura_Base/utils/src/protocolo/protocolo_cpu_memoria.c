#include "protocolo_cpu_memoria.h"
#include <paquete/paquete.h>

void enviar_fetch_instruccion(int socket_memoria, t_mem_fetch* req)
{
    t_paquete* p = crear_paquete(FETCH_INSTRUCCION);
    agregar_entero_a_paquete(p, req->pid);
    agregar_entero_a_paquete(p, req->pc);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

void enviar_traduccion_pagina(int socket_memoria, t_mem_traducir_pagina* req)
{
    t_paquete* p = crear_paquete(ACCESO_TABLA);
    agregar_entero_a_paquete(p, req->pid);
    agregar_entero_a_paquete(p, req->pagina);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

void enviar_lectura_memoria(int socket_memoria, t_mem_rw* req)
{
    t_paquete* p = crear_paquete(LEER_MEMORIA);
    agregar_entero_a_paquete(p, req->pid);
    agregar_entero_a_paquete(p, req->direccion_fisica);
    agregar_entero_a_paquete(p, req->tamanio);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

void enviar_escritura_memoria(int socket_memoria, t_mem_rw* req)
{
    t_paquete* p = crear_paquete(ESCRIBIR_MEMORIA);
    agregar_entero_a_paquete(p, req->pid);
    agregar_entero_a_paquete(p, req->direccion_fisica);
    agregar_entero_a_paquete(p, req->tamanio);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

char* recibir_instruccion(int socket_memoria)
{
    t_list* valores = recibir_paquete(socket_memoria);
    char* inst = strdup((char*)list_get(valores, 0));
    list_destroy_and_destroy_elements(valores, free);
    return inst;
}

uint32_t recibir_frame(int socket_memoria)
{
    t_list* valores = recibir_paquete(socket_memoria);
    uint32_t frame = *(uint32_t*)list_get(valores, 0);
    list_destroy_and_destroy_elements(valores, free);
    return frame;
}
