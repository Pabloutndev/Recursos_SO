# Video 23 - DialFS: Filesystem

**Duración estimada:** 15-20 minutos  
**Bloque:** Entrada/Salida

---

## Conceptos

### Sistema de Archivos
- Organizar datos en disco
- Metadata: nombre, tamaño, ubicación
- Bitmap: bloques libres/ocupados

### DialFS
- Filesystem custom para el TP
- Bloques de tamaño fijo
- FCB (File Control Block) por archivo

---

## Código

### Estructura
```c
typedef struct {
    char nombre[256];
    uint32_t bloque_inicio;
    uint32_t tamanio;
} t_fcb;
```

### Bitmap
**Archivo:** `entradasalida/src/interfaces/dialfs_bitmap.c`

```c
t_bitarray* bitmap;
FILE* archivo_bloques;

void dialfs_bitmap_init() {
    archivo_bloques = fopen(config.PATH_BLOQUES, "r+b");
    // ... inicializar bitmap
}

int dialfs_buscar_bloque_libre() {
    for (int i = 0; i < config.CANTIDAD_BLOQUES; i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            return i;
        }
    }
    return -1;
}

void dialfs_ocupar_bloque(int bloque) {
    bitarray_set_bit(bitmap, bloque);
}
```

### CREATE
**Archivo:** `entradasalida/src/interfaces/dialfs.c`

```c
void dialfs_create(char* nombre_archivo) {
    // Crear FCB
    t_fcb* fcb = malloc(sizeof(t_fcb));
    strcpy(fcb->nombre, nombre_archivo);
    fcb->bloque_inicio = 0;
    fcb->tamanio = 0;
    
    // Guardar metadata
    char* path_fcb = string_from_format("%s/%s.fcb", config.PATH_METADATA, nombre_archivo);
    FILE* archivo = fopen(path_fcb, "wb");
    fwrite(fcb, sizeof(t_fcb), 1, archivo);
    fclose(archivo);
    
    log_info(logger, "DialFS: CREATE %s", nombre_archivo);
    
    free(path_fcb);
    free(fcb);
}
```

### WRITE
```c
void dialfs_write(char* nombre_archivo, void* datos, uint32_t tamanio, uint32_t offset) {
    // Cargar FCB
    t_fcb* fcb = dialfs_cargar_fcb(nombre_archivo);
    
    // Calcular bloques necesarios
    int bloques_necesarios = (offset + tamanio + config.TAM_BLOQUE - 1) / config.TAM_BLOQUE;
    
    // Asignar bloques si es necesario
    if (bloques_necesarios > fcb->tamanio / config.TAM_BLOQUE) {
        // ... asignar más bloques contiguos
    }
    
    // Escribir datos
    fseek(archivo_bloques, fcb->bloque_inicio * config.TAM_BLOQUE + offset, SEEK_SET);
    fwrite(datos, tamanio, 1, archivo_bloques);
    fflush(archivo_bloques);
    
    // Actualizar FCB
    if (offset + tamanio > fcb->tamanio) {
        fcb->tamanio = offset + tamanio;
        dialfs_guardar_fcb(fcb);
    }
    
    log_info(logger, "DialFS: WRITE %s, %d bytes en offset %d", 
        nombre_archivo, tamanio, offset);
    
    free(fcb);
}
```

---

## Demo

### Proceso
```
IO_FS_CREATE FS archivo.txt
IO_FS_WRITE FS archivo.txt 0 10 0
IO_FS_READ FS archivo.txt 100 10 0
EXIT
```

### Logs
```
[IO FS] CREATE archivo.txt
[IO FS] FCB creado: archivo.txt (tam=0)
[IO FS] WRITE archivo.txt: 10 bytes en offset 0
[IO FS] Bloques asignados: 1
[IO FS] Datos escritos en bloque 5
[IO FS] FCB actualizado: tam=10
[IO FS] READ archivo.txt: 10 bytes desde offset 0
[IO FS] Datos leídos de bloque 5
[MEMORIA] Write: PID=1, dir=100, 10 bytes
```

---

## Puntos Clave

1. **Bloques contiguos:** Simplifica gestión
2. **FCB:** Metadata separada de datos
3. **Bitmap:** Gestión eficiente de espacio

---

## Siguiente Video

Veremos **Test Suite: cómo verificar el SO**.
