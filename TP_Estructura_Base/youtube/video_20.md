# Video 20 - Detección de Deadlock: Grafo de Espera

**Duración estimada:** 15-20 minutos  
**Bloque:** Sincronización

---

## Conceptos

### Deadlock
- Conjunto de procesos esperándose mutuamente
- Nunca progresarán
- **Condiciones de Coffman:**
  1. Exclusión mutua
  2. Hold and wait
  3. No preemption
  4. Espera circular

### Grafo de Espera
- Nodos: procesos
- Aristas: A espera recurso de B
- **Ciclo = Deadlock**

---

## Código

### Construir grafo
```c
t_grafo* construir_grafo_espera() {
    t_grafo* grafo = grafo_crear();
    
    // Para cada recurso
    DICTIONARY_ITERATE(recursos, char*, nombre, t_recurso*, recurso) {
        
        // Procesos bloqueados en este recurso
        QUEUE_ITERATE(recurso->cola_bloqueados, t_pcb*, bloqueado) {
            
            // Buscar quién tiene el recurso
            t_pcb* duenio = buscar_duenio_recurso(nombre);
            
            if (duenio) {
                // Arista: bloqueado → dueño
                grafo_agregar_arista(grafo, bloqueado->pid, duenio->pid, nombre);
                
                log_trace(logger, "Arista: PID=%d espera a PID=%d (recurso %s)", 
                    bloqueado->pid, duenio->pid, nombre);
            }
        }
    }
    
    return grafo;
}
```

### Detectar ciclo (DFS)
```c
bool detectar_ciclo_dfs(t_grafo* grafo, uint32_t nodo, t_set* visitados, t_set* en_stack, t_list* camino) {
    set_add(visitados, nodo);
    set_add(en_stack, nodo);
    list_add(camino, nodo);
    
    t_list* vecinos = grafo_obtener_vecinos(grafo, nodo);
    
    LIST_ITERATE(vecinos, uint32_t, vecino) {
        if (!set_contains(visitados, vecino)) {
            if (detectar_ciclo_dfs(grafo, vecino, visitados, en_stack, camino)) {
                return true;  // Ciclo encontrado
            }
        }
        else if (set_contains(en_stack, vecino)) {
            // Vecino ya está en el stack → ciclo!
            list_add(camino, vecino);  // Cerrar ciclo
            return true;
        }
    }
    
    set_remove(en_stack, nodo);
    list_remove(camino, list_size(camino) - 1);
    
    return false;
}
```

### Detectar deadlock
```c
void deteccion_deadlock() {
    t_grafo* grafo = construir_grafo_espera();
    
    t_set* visitados = set_create();
    t_set* en_stack = set_create();
    t_list* camino = list_create();
    
    // Probar desde cada nodo
    DICTIONARY_ITERATE(procesos_pcb, char*, key, t_pcb*, pcb) {
        if (!set_contains(visitados, pcb->pid)) {
            if (detectar_ciclo_dfs(grafo, pcb->pid, visitados, en_stack, camino)) {
                log_warning(logger, "¡DEADLOCK DETECTADO!");
                reportar_deadlock(camino);
                break;
            }
        }
    }
    
    grafo_destruir(grafo);
    set_destroy(visitados);
    set_destroy(en_stack);
    list_destroy(camino);
}
```

---

## Demo: Test 11 (Deadlock A↔B)

### deadlock_a.txt
```
WAIT RA
WAIT RB
SIGNAL RB
SIGNAL RA
EXIT
```

### deadlock_b.txt
```
WAIT RB
WAIT RA
SIGNAL RA
SIGNAL RB
EXIT
```

### Logs
```
[KERNEL] PID=1: WAIT RA → adquirido
[KERNEL] PID=2: WAIT RB → adquirido
[KERNEL] PID=1: WAIT RB → bloqueado (RB lo tiene PID=2)
[KERNEL] PID=2: WAIT RA → bloqueado (RA lo tiene PID=1)
[KERNEL] Detección de deadlock...
[KERNEL] Grafo: PID=1 → PID=2 (RB)
[KERNEL] Grafo: PID=2 → PID=1 (RA)
[KERNEL] ¡DEADLOCK! Ciclo: 1 → 2 → 1
```

---

## Puntos Clave

1. **Grafo dirigido:** Espera tiene dirección
2. **DFS:** Algoritmo clásico para ciclos
3. **Cuándo detectar:** Después de cada WAIT que bloquea

---

## Siguiente Video

Veremos **Algoritmo del Banquero** como alternativa.
