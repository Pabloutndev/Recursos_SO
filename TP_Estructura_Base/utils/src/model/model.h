#ifndef COMMON_MODELOS_H_
#define COMMON_MODELOS_H_

#include <stdbool.h>
#include <stdint.h>

/* Sentinel para errores de traducción MMU (frame 0 es válido) */
#define TRADUCCION_ERROR UINT32_MAX

/* =========================
   REGISTROS Y CONTEXTO
   ========================= */
typedef enum {
    REG_AX, REG_BX, REG_CX, REG_DX,
    REG_EAX, REG_EBX, REG_ECX, REG_EDX,
    REG_SI, REG_DI,
} reg_id_t;

typedef struct {
    uint8_t  AX, BX, CX, DX;
    uint32_t EAX, EBX, ECX, EDX;
    uint32_t SI, DI;
} registros_t;

typedef struct {
    uint32_t pid;
    uint32_t pc;
    registros_t registros;
} t_contexto_cpu;

/* =========================
   MOTIVOS / DESALOJOS
   ========================= */

typedef enum {
    MOTIVO_FIN_QUANTUM,
    MOTIVO_IO,
    MOTIVO_EXIT,
    MOTIVO_SEGFAULT,
    MOTIVO_DESALOJO,
    CPU_CONTINUAR
} t_motivo_desalojo;


/* =========================
   MEMORIA
   ========================= */

typedef struct {
    uint32_t pid;
    char path[256]; // solo para init
} t_mem_init_proceso;

typedef struct {
    uint32_t pid;
} t_mem_fin_proceso;

typedef struct {
    uint32_t pid;
    uint32_t direccion_logica;
} t_mem_traducir;

typedef struct {
    uint32_t pid;
    uint32_t direccion_logica;
    uint32_t size;
} t_mem_read;

typedef struct {
    uint32_t pid;
    uint32_t direccion_logica;
    uint32_t size;
    void* buffer;
} t_mem_write;

typedef struct {
    uint32_t pid;
    uint32_t pc;
} t_mem_fetch;

/* RESPONSE */
typedef struct {
    bool ok;
    uint32_t direccion_fisica;
} t_mem_respuesta_traduccion;

typedef struct {
    bool ok;
    uint32_t size;
    void* data;
} t_mem_respuesta_lectura;

/* =========================
   IO
   ========================= */

typedef struct {
    uint32_t pid;
    uint32_t tiempo;
} t_io_sleep;

typedef struct {
    uint32_t pid;
    char path[256];
    uint32_t offset;
    uint32_t size;
    uint32_t direccion_logica; // direccion de memoria para READ/WRITE
} t_io_fs_write;

typedef struct {
    uint32_t pid;
    char path[256];
} t_io_fs_create;

/* STDIN/STDOUT: dirección lógica + tamaño para leer/escribir en memoria */
typedef struct {
    uint32_t pid;
    uint32_t direccion_logica;
    uint32_t size;
    char interfaz[64];
} t_io_stdin_read;

typedef struct {
    uint32_t pid;
    uint32_t direccion_logica;
    uint32_t size;
    char interfaz[64];
} t_io_stdout_write;

/* Resize de proceso */
typedef struct {
    uint32_t pid;
    uint32_t nuevo_tamanio;
} t_mem_resize;

#endif
