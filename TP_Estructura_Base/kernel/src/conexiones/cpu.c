#include <conexiones/cpu.h>
#include <mod_kernel.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <serializacion/serializacion.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <peticiones/interrupciones.h>
#include <peticiones/dispatch.h>
#include <planificacion/planificacion.h>
#include <adaptadores/kernel_io_adapter.h>
#include <conexiones/memoria.h>
#include <conexiones/io.h>
#include <pthread.h>
#include <pcb/pcb.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <stdlib.h>
#include <string.h>

// Variables globales para sockets match mod_kernel.c
extern int socket_dispatch;
extern int socket_interrupt;
extern t_log* logger;
extern t_log* loggerError;

extern t_list* cola_exit;
extern t_list* cola_exec;
extern pthread_mutex_t mutex_exec;

static pthread_t hilo_interrupt;

static void* escuchar_interrupt(void* arg);
static void manejar_segfault_static(t_contexto_cpu* ctx);
static void manejar_bloqueo_io_static(t_contexto_cpu* ctx);
static void manejar_io_stdin_read(t_contexto_cpu* ctx);
static void manejar_io_stdout_write(t_contexto_cpu* ctx);

void enviar_contexto_a_cpu(t_contexto_cpu* ctx) {
    // Usamos el helper de protocolo que refactorizamos
    enviar_contexto(socket_dispatch, ctx, OP_PROCESO_EXEC);
}

void enviar_interrupcion_a_cpu(int pid, int motivo) {
    // TODO: Confirmar si CPU espera payload. Por ahora usamos helper simple.
    enviar_interrupcion_cpu(socket_interrupt);
}

t_contexto_cpu* recibir_contexto_de_cpu(void) {
    // Funcion no utilizada en modelo asincrono con hilo dedicado
    return NULL; 
}

void conectar_cpu(char* ip, char* puerto_dispatch, char* puerto_interrupt)
{
    // 1. Dispatch
    socket_dispatch = crear_conexion(ip, puerto_dispatch);
    if(socket_dispatch < 0) {
        log_error(loggerError, "Error conectando a CPU dispatch");
        exit(EXIT_FAILURE);
    }
    // Handshake
    handshake_cliente(socket_dispatch, OP_HANDSHAKE, OP_OK, logger);

    // 2. Interrupt
    socket_interrupt = crear_conexion(ip, puerto_interrupt);
    if(socket_interrupt < 0) {
        log_error(loggerError, "Error conectando a CPU interrupt");
        exit(EXIT_FAILURE);
    }
    // Handshake
    handshake_cliente(socket_interrupt, OP_HANDSHAKE, OP_OK, logger);

    // 3. Threads
    pthread_create(&hilo_interrupt, NULL, escuchar_interrupt, NULL);

    log_info(logger, "CPU conectada (dispatch + interrupt)");
}

// La función escuchar_dispatch ahora es llamada sincrónicamente por el planificador de corto plazo
void atender_dispatch_cpu(void)
{
    t_paquete* paquete = recibir_paquete(socket_dispatch);
    if (paquete == NULL) {
        log_error(loggerError, "CPU Dispatch desconectado");
        return;
    }

    switch (paquete->codigo_operacion) {

    case OP_FIN_DE_QUANTUM: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            manejar_fin_quantum(ctx);
            free(ctx);
        }
        break;
    }

    case OP_CPU_FIN_PROCESO: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            manejar_fin_proceso(ctx);
            free(ctx);
        }
        break;
    }

    case OP_IO_SLEEP: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            manejar_bloqueo_io_static(ctx);
            free(ctx);
        }
        break;
    }

    case OP_IO_STDIN_READ: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            manejar_io_stdin_read(ctx);
            free(ctx);
        }
        break;
    }

    case OP_IO_STDOUT_WRITE: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            manejar_io_stdout_write(ctx);
            free(ctx);
        }
        break;
    }

    case OP_WAIT_RECURSO: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        char* nombre_recurso = paquete_read_string(paquete);
        if (ctx) {
            manejar_wait_recurso(ctx, nombre_recurso);
            free(ctx);
        }
        if (nombre_recurso) free(nombre_recurso);
        break;
    }

    case OP_SIGNAL_RECURSO: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        char* nombre_recurso = paquete_read_string(paquete);
        if (ctx) {
            manejar_signal_recurso(ctx, nombre_recurso);
            free(ctx);
        }
        if (nombre_recurso) free(nombre_recurso);
        break;
    }

    case OP_SEGFAULT: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            manejar_segfault_static(ctx);
            free(ctx);
        }
        break;
    }

    default:
        log_warning(logger, "OpCode desconocido CPU->Kernel: %d", paquete->codigo_operacion);
        break;
    }

    paquete_destroy(paquete);
}

static void* escuchar_interrupt(void* _)
{
    while (1) {
        t_paquete* paquete = recibir_paquete(socket_interrupt);
        if (paquete == NULL) {
            log_error(loggerError, "CPU Interrupt desconectado");
            break;
        }

        // Kernel NO deberia recibir nada por interrupt channel, salvo quizas ACKs?
        log_warning(logger, "Kernel recibió algo por interrupt (ignorado): %d", paquete->codigo_operacion);
        
        paquete_destroy(paquete);
    }

    return NULL;
}

/* ========================================
 * MANEJADORES DE EVENTOS DE CPU
 * ======================================== */

static void manejar_bloqueo_io_static(t_contexto_cpu* ctx)
{
    manejar_bloqueo_io(ctx);
}


static void manejar_segfault_static(t_contexto_cpu* ctx)
{
    if (!ctx) return;
    
    log_error(loggerError, "CPU: Segmentation Fault para PID=%d, PC=%u", ctx->pid, ctx->pc);
    
    // Actualizar PC en PCB
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* pcb = (t_pcb*) list_get(cola_exec, i);
        if (pcb && pcb->pid == (uint32_t)ctx->pid) {
            pcb->program_counter = ctx->pc;
            pcb->registros = ctx->registros;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);
    
    // Finalizar por segfault
    manejar_interrupcion(ctx->pid, "EXIT");

    // Liberar recursos
    solicitar_fin_proceso_memoria(ctx->pid);
}

/* ========================================
 * MANEJADORES IO STDIN / STDOUT
 * ======================================== */

static void manejar_io_stdin_read(t_contexto_cpu* ctx)
{
    if (!ctx) return;

    log_info(logger, "CPU: IO_STDIN_READ para PID=%u, PC=%u", ctx->pid, ctx->pc);

    // 1. Buscar PCB en EXEC y actualizar contexto
    t_pcb* pcb = NULL;
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* p = (t_pcb*) list_get(cola_exec, i);
        if (p && p->pid == (uint32_t)ctx->pid) {
            pcb = p;
            pcb->program_counter = ctx->pc;
            pcb->registros = ctx->registros;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    if (!pcb) {
        log_error(loggerError, "IO_STDIN_READ: PCB no encontrado para PID=%u", ctx->pid);
        return;
    }

    // 2. Construir request de IO STDIN con datos de registros
    //    Convención TP: dirección lógica en EAX, tamaño en EBX,
    //    nombre de interfaz se pasa como string desde la instrucción.
    //    Como el contexto no trae el nombre, usamos un genérico "STDIN".
    t_io_stdin_read req;
    req.pid = ctx->pid;
    req.direccion_logica = ctx->registros.EAX;
    req.size = ctx->registros.EBX;
    strncpy(req.interfaz, "STDIN", sizeof(req.interfaz) - 1);
    req.interfaz[sizeof(req.interfaz) - 1] = '\0';

    // 3. Buscar interfaz y enviar operación
    int socket_io = obtener_socket_interfaz(req.interfaz);
    if (socket_io >= 0) {
        enviar_io_stdin_read(socket_io, &req);
        log_info(logger, "IO_STDIN_READ: Enviado a interfaz %s (PID=%u, DIR=%u, SIZE=%u)",
                 req.interfaz, req.pid, req.direccion_logica, req.size);
    } else {
        log_error(loggerError, "IO_STDIN_READ: Interfaz '%s' no encontrada para PID=%u",
                  req.interfaz, req.pid);
    }

    // 4. Bloquear proceso (EXEC -> BLOCKED)
    manejar_interrupcion(ctx->pid, "IO");
}

static void manejar_io_stdout_write(t_contexto_cpu* ctx)
{
    if (!ctx) return;

    log_info(logger, "CPU: IO_STDOUT_WRITE para PID=%u, PC=%u", ctx->pid, ctx->pc);

    // 1. Buscar PCB en EXEC y actualizar contexto
    t_pcb* pcb = NULL;
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* p = (t_pcb*) list_get(cola_exec, i);
        if (p && p->pid == (uint32_t)ctx->pid) {
            pcb = p;
            pcb->program_counter = ctx->pc;
            pcb->registros = ctx->registros;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    if (!pcb) {
        log_error(loggerError, "IO_STDOUT_WRITE: PCB no encontrado para PID=%u", ctx->pid);
        return;
    }

    // 2. Construir request de IO STDOUT con datos de registros
    t_io_stdout_write req;
    req.pid = ctx->pid;
    req.direccion_logica = ctx->registros.EAX;
    req.size = ctx->registros.EBX;
    strncpy(req.interfaz, "STDOUT", sizeof(req.interfaz) - 1);
    req.interfaz[sizeof(req.interfaz) - 1] = '\0';

    // 3. Buscar interfaz y enviar operación
    int socket_io = obtener_socket_interfaz(req.interfaz);
    if (socket_io >= 0) {
        enviar_io_stdout_write(socket_io, &req);
        log_info(logger, "IO_STDOUT_WRITE: Enviado a interfaz %s (PID=%u, DIR=%u, SIZE=%u)",
                 req.interfaz, req.pid, req.direccion_logica, req.size);
    } else {
        log_error(loggerError, "IO_STDOUT_WRITE: Interfaz '%s' no encontrada para PID=%u",
                  req.interfaz, req.pid);
    }

    // 4. Bloquear proceso (EXEC -> BLOCKED)
    manejar_interrupcion(ctx->pid, "IO");
}