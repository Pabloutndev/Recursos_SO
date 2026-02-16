# Video 11 - Ciclo Fetch-Decode-Execute

**Duración estimada:** 15-20 minutos  
**Bloque:** La CPU

---

## Conceptos a Explicar

### Ciclo de instrucción
- **Fetch:** Traer instrucción de memoria
- **Decode:** Interpretar qué hacer
- **Execute:** Realizar la operación
- Loop infinito hasta EXIT o interrupción

### Program Counter (PC)
- Apunta a la próxima instrucción
- Se incrementa automáticamente
- Puede modificarse (JNZ, saltos)

### Instruction Set Architecture (ISA)
- Conjunto de instrucciones que la CPU entiende
- Nuestro TP: custom ISA simplificado
- Compara con: x86, ARM, RISC-V

---

## Código y Demostración

### 1. Loop principal de CPU
**Archivo:** `cpu/src/ciclo_instruccion/ciclo.c`

```c
t_resultado ciclo_instruccion_ejecutar(t_contexto* ctx) {
    log_info(logger, "Iniciando ciclo de instrucción para PID=%d", ctx->pid);
    
    while (true) {
        // 1. Chequear interrupciones ANTES de fetch
        if (interrupcion_pendiente()) {
            log_info(logger, "Interrupción pendiente para PID=%d", ctx->pid);
            return crear_resultado_interrupcion(ctx);
        }
        
        // 2. FETCH: traer instrucción de Memoria
        char* instruccion_str = fetch_instruccion(ctx->pid, ctx->pc);
        
        if (!instruccion_str) {
            log_error(logger, "Error en fetch para PID=%d, PC=%d", 
                ctx->pid, ctx->pc);
            return crear_resultado_error(ctx, ERROR_FETCH);
        }
        
        log_trace(logger, "Fetch: PID=%d, PC=%d, '%s'", 
            ctx->pid, ctx->pc, instruccion_str);
        
        // 3. DECODE: parsear instrucción
        t_instruccion* instr = decode_instruccion(instruccion_str);
        free(instruccion_str);
        
        if (!instr) {
            log_error(logger, "Error en decode para PID=%d", ctx->pid);
            return crear_resultado_error(ctx, ERROR_DECODE);
        }
        
        // 4. EXECUTE: ejecutar según tipo
        t_resultado resultado = execute_instruccion(ctx, instr);
        destruir_instruccion(instr);
        
        // 5. Incrementar PC (salvo que la instrucción lo haya modificado)
        if (!resultado.modifico_pc) {
            ctx->pc++;
        }
        
        // 6. Chequear si debe terminar el ciclo
        if (resultado.terminar_ciclo) {
            return resultado;
        }
    }
}
```

### 2. Fetch
**Archivo:** `cpu/src/ciclo_instruccion/fetch.c`

```c
char* fetch_instruccion(uint32_t pid, uint32_t pc) {
    log_trace(logger, "Solicitando instrucción: PID=%d, PC=%d", pid, pc);
    
    // Pedir a Memoria
    t_paquete* paq = crear_paquete(MEMORIA_FETCH_INSTRUCCION);
    escribir_uint32(paq, pid);
    escribir_uint32(paq, pc);
    enviar_paquete(paq, socket_memoria);
    destruir_paquete(paq);
    
    // Recibir respuesta
    op_code op = recibir_operacion(socket_memoria);
    
    if (op != MEMORIA_OK) {
        log_error(logger, "Memoria devolvió error en fetch");
        return NULL;
    }
    
    t_paquete* respuesta = recibir_paquete(socket_memoria);
    char* instruccion = leer_string(respuesta);
    destruir_paquete(respuesta);
    
    log_trace(logger, "Instrucción recibida: '%s'", instruccion);
    
    return instruccion;
}
```

### 3. Decode
**Archivo:** `cpu/src/ciclo_instruccion/decode.c`

```c
typedef enum {
    INSTR_SET,
    INSTR_SUM,
    INSTR_SUB,
    INSTR_JNZ,
    INSTR_MOV_IN,
    INSTR_MOV_OUT,
    INSTR_RESIZE,
    INSTR_IO_GEN_SLEEP,
    INSTR_WAIT,
    INSTR_SIGNAL,
    INSTR_EXIT
} tipo_instruccion;

typedef struct {
    tipo_instruccion tipo;
    char* parametros[4];  // Máximo 4 parámetros
    int cantidad_parametros;
} t_instruccion;

t_instruccion* decode_instruccion(char* str) {
    t_instruccion* instr = malloc(sizeof(t_instruccion));
    
    char** tokens = string_split(str, ' ');
    char* nombre = tokens[0];
    
    // Identificar tipo
    if (strcmp(nombre, "SET") == 0) {
        instr->tipo = INSTR_SET;
        instr->parametros[0] = string_duplicate(tokens[1]);  // registro
        instr->parametros[1] = string_duplicate(tokens[2]);  // valor
        instr->cantidad_parametros = 2;
    }
    else if (strcmp(nombre, "SUM") == 0) {
        instr->tipo = INSTR_SUM;
        instr->parametros[0] = string_duplicate(tokens[1]);  // destino
        instr->parametros[1] = string_duplicate(tokens[2]);  // origen
        instr->cantidad_parametros = 2;
    }
    else if (strcmp(nombre, "EXIT") == 0) {
        instr->tipo = INSTR_EXIT;
        instr->cantidad_parametros = 0;
    }
    // ... otros casos
    
    string_array_destroy(tokens);
    
    return instr;
}
```

### 4. Execute
**Archivo:** `cpu/src/ciclo_instruccion/execute.c`

```c
t_resultado execute_instruccion(t_contexto* ctx, t_instruccion* instr) {
    t_resultado res;
    res.terminar_ciclo = false;
    res.modifico_pc = false;
    
    switch (instr->tipo) {
        case INSTR_SET:
            return ejecutar_set(ctx, instr);
        
        case INSTR_SUM:
            return ejecutar_sum(ctx, instr);
        
        case INSTR_JNZ:
            return ejecutar_jnz(ctx, instr);  // Puede modificar PC
        
        case INSTR_MOV_IN:
            return ejecutar_mov_in(ctx, instr);  // Acceso a memoria
        
        case INSTR_IO_GEN_SLEEP:
            return ejecutar_io(ctx, instr);  // Bloquea proceso
        
        case INSTR_EXIT:
            return ejecutar_exit(ctx);  // Termina ciclo
        
        default:
            log_error(logger, "Instrucción desconocida: %d", instr->tipo);
            return crear_resultado_error(ctx, ERROR_INSTRUCCION_INVALIDA);
    }
}
```

---

## Implementación de Instrucciones

### SET
**Archivo:** `cpu/src/instrucciones/operaciones.c`

```c
t_resultado ejecutar_set(t_contexto* ctx, t_instruccion* instr) {
    char* registro = instr->parametros[0];
    int valor = atoi(instr->parametros[1]);
    
    registros_escribir(&ctx->registros, registro, valor);
    
    log_trace(logger, "SET %s %d", registro, valor);
    
    t_resultado res;
    res.terminar_ciclo = false;
    res.modifico_pc = false;
    return res;
}
```

### SUM
```c
t_resultado ejecutar_sum(t_contexto* ctx, t_instruccion* instr) {
    char* destino = instr->parametros[0];
    char* origen = instr->parametros[1];
    
    uint32_t val_destino = registros_leer(&ctx->registros, destino);
    uint32_t val_origen = registros_leer(&ctx->registros, origen);
    
    uint32_t resultado = val_destino + val_origen;
    
    registros_escribir(&ctx->registros, destino, resultado);
    
    log_trace(logger, "SUM %s %s (resultado=%d)", destino, origen, resultado);
    
    t_resultado res;
    res.terminar_ciclo = false;
    res.modifico_pc = false;
    return res;
}
```

### JNZ (Jump if Not Zero)
```c
t_resultado ejecutar_jnz(t_contexto* ctx, t_instruccion* instr) {
    char* registro = instr->parametros[0];
    int offset = atoi(instr->parametros[1]);
    
    uint32_t valor = registros_leer(&ctx->registros, registro);
    
    if (valor != 0) {
        ctx->pc += offset;
        log_trace(logger, "JNZ %s %d (tomado, nuevo PC=%d)", 
            registro, offset, ctx->pc);
        
        t_resultado res;
        res.terminar_ciclo = false;
        res.modifico_pc = true;  // No incrementar PC en el loop
        return res;
    } else {
        log_trace(logger, "JNZ %s %d (no tomado)", registro, offset);
        
        t_resultado res;
        res.terminar_ciclo = false;
        res.modifico_pc = false;
        return res;
    }
}
```

### EXIT
```c
t_resultado ejecutar_exit(t_contexto* ctx) {
    log_info(logger, "EXIT ejecutado para PID=%d", ctx->pid);
    
    t_resultado res;
    res.terminar_ciclo = true;
    res.motivo = MOTIVO_EXIT;
    return res;
}
```

---

## Demo: Seguir instrucción por instrucción

### Archivo: `test_ciclo.txt`
```
SET AX 10
SET BX 20
SUM AX BX
JNZ AX -1
EXIT
```

### Logs esperados
```
[CPU] Iniciando ciclo PID=1
[CPU] Fetch: PC=0, 'SET AX 10'
[CPU] Decode: SET (2 params)
[CPU] Execute: SET AX 10
[CPU] PC: 0 → 1
[CPU] Fetch: PC=1, 'SET BX 20'
[CPU] Execute: SET BX 20
[CPU] PC: 1 → 2
[CPU] Fetch: PC=2, 'SUM AX BX'
[CPU] Execute: SUM AX BX (resultado=30)
[CPU] PC: 2 → 3
[CPU] Fetch: PC=3, 'JNZ AX -1'
[CPU] Execute: JNZ AX -1 (tomado, nuevo PC=2)
[CPU] Fetch: PC=2, 'SUM AX BX'
[CPU] Execute: SUM AX BX (resultado=50)
... (loop infinito hasta interrupción)
```

---

## Puntos Clave a Destacar

1. **Fetch es bloqueante:** CPU espera a Memoria
2. **PC es crítico:** Determina el flujo del programa
3. **Interrupciones checked:** Antes de cada fetch
4. **Resultados:** Cada instrucción devuelve si debe terminar el ciclo

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos los **registros** en detalle
- Tabla completa de instrucciones
- WAIT/SIGNAL: instrucciones especiales
