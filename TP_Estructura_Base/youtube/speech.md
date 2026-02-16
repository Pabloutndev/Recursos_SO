# Speech Guide - Curso Sistemas Operativos (Videos 1-25)

Este documento contiene el guión narrativo completo para cada video del curso.

---

## Video 1: Introducción al Sistema Operativo (15-20 min)

**Bienvenida:**
"Hola a todos, bienvenidos al curso completo de Sistemas Operativos. En este primer video vamos a responder la pregunta fundamental: ¿qué es un Sistema Operativo y por qué existe?"

**Desarrollo:**
"Un Sistema Operativo es el software que actúa como intermediario entre el hardware de tu computadora y las aplicaciones que usás todos los días. Sin él, cada programa tendría que saber cómo hablar directamente con el disco duro, con la memoria RAM, con la tarjeta de red. Sería un caos total.

El SO resuelve tres problemas fundamentales: primero, la gestión eficiente de recursos limitados como CPU, memoria y dispositivos. Segundo, la multiprogramación, es decir, ejecutar varios procesos al mismo tiempo de forma segura. Y tercero, la abstracción del hardware, dándole a los programadores una interfaz sencilla para que no tengan que preocuparse por los detalles de bajo nivel.

Ahora, hablemos de nuestro proyecto. Vamos a construir un SO completamente funcional dividido en cinco módulos independientes: el Kernel que coordina todo, la CPU que ejecuta instrucciones, la Memoria que gestiona RAM y swap, el módulo de Entrada/Salida para dispositivos, y la Consola que es nuestra interfaz de usuario.

Lo interesante es que cada módulo es un proceso separado que se comunica con los demás a través de sockets TCP. Esto simula un sistema distribuido y nos permite arrancar o detener módulos de forma independiente.

Veamos el flujo de vida de un proceso: nace en estado NEW cuando lo creamos, pasa a READY cuando está listo para ejecutar, se mueve a EXEC cuando la CPU lo ejecuta, puede ir a BLOCKED si necesita esperar por algo como I/O, y finalmente termina en EXIT.

Algo fundamental: cada módulo se configura con un archivo .config. Ahí definís IPs, puertos, algoritmo de planificación, quantum, grado de multiprogramación, recursos disponibles. Es texto plano clave=valor, muy fácil de modificar sin recompilar. Por ejemplo, en kernel.config podés cambiar de Round Robin a FIFO con solo cambiar una línea.

Otro pilar del proyecto es el logging. Cada módulo genera logs detallados de todo lo que hace: creación de procesos, cambios de estado, traducciones de memoria, operaciones de I/O. Estos logs son tu herramienta principal de debugging. Cuando algo falla, no vas a estar poniendo breakpoints en 4 procesos a la vez, vas a leer los logs.

Ahora les muestro cómo compilar y ejecutar el proyecto. Tenemos un Makefile para cada módulo, podemos hacer 'make all' para compilar todo, y luego levantamos cada módulo con su archivo de configuración correspondiente."

**Cierre:**
"En el próximo video vamos a profundizar en cómo se comunican estos módulos entre sí, viendo el protocolo de comunicación basado en sockets TCP y serialización de datos."

---

## Video 2: Protocolo de Comunicación (10-15 min)

**Introducción:**
"Bienvenidos al video 2. Ya vimos que nuestro SO tiene cinco módulos separados. Hoy vamos a entender cómo se comunican entre ellos usando Inter-Process Communication o IPC."

**Desarrollo:**
"Hay varias formas de hacer IPC: pipes, memoria compartida, message queues, pero nosotros elegimos sockets TCP. ¿Por qué? Porque nos da una conexión confiable y orientada a flujo, y además, en teoría, cada módulo podría estar en una máquina diferente de la red.

El problema con enviar datos por red es que no podés mandar directamente una estructura de C. Tenés que convertirla a bytes, enviarla, y del otro lado reconstruirla. A eso se le llama serialización y deserialización.

Nuestro protocolo funciona así: primero definimos opcodes, que son números que identifican qué tipo de mensaje es. Por ejemplo, el opcode 1 puede ser un handshake, el 100 puede ser un fetch de instrucción de la CPU, el 200 una creación de proceso en Memoria. Tenemos rangos asignados por módulo para mantener el orden.

Cada paquete que enviamos tiene tres partes: el opcode que dice qué es, el tamaño de los datos, y los datos en sí. La biblioteca utils que armamos encapsula toda esta complejidad. Tenemos funciones como crear_paquete, escribir_int32, escribir_string, enviar_paquete, y del otro lado recibir y leer.

Les muestro un ejemplo concreto: cuando la Consola envía el comando RUN, crea un paquete con el opcode RUN_PROCESO, escribe el path del archivo como string, y lo envía al Kernel. El Kernel recibe la operación, lee el string, y ejecuta la lógica correspondiente.

El handshake es la primera comunicación. Cuando un cliente se conecta, envía su nombre de módulo, y el servidor responde OK si acepta la conexión. Así sabemos que estamos hablando con quien creemos."

**Cierre:**
"En el próximo video vamos a ver la Consola en detalle, cómo funciona el loop de lectura de comandos y qué comandos tenemos disponibles para controlar el sistema."

---

## Video 3: La Consola - Interfaz de Usuario (5-10 min)

**Introducción:**
"Video 3, vamos a hablar de la Consola, que es nuestra interfaz humano-máquina. Es la forma en que vos, como usuario, le decís al Sistema Operativo qué hacer."

**Desarrollo:**
"La Consola es lo que en Unix se llama un shell. Implementa un patrón REPL: Read-Eval-Print-Loop. Lee un comando del usuario, lo evalúa ejecutándolo, imprime el resultado, y repite el ciclo indefinidamente.

Usamos la biblioteca readline de GNU que nos da edición de línea, historial de comandos con la flechita para arriba, y autocompletado. Es lo que hace que la experiencia sea cómoda.

Los comandos disponibles son: RUN para crear un proceso nuevo, y opcionalmente podés pasarle una prioridad, por ejemplo 'RUN test.txt 5' donde 0 es la máxima prioridad. KILL para finalizar un proceso forzosamente por PID. PS para listar todos los procesos activos con su PID y estado. ALGORITMO para cambiar el algoritmo de planificación en caliente sin reiniciar, soporta FIFO, RR, VRR, HRRN y PRIORIDAD. START y PAUSE para iniciar o pausar la planificación de largo plazo. DESALOJAR para sacar un proceso de la CPU forzosamente. HELP para ver la ayuda. Y EXIT para cerrar la consola.

Internamente, la Consola es súper simple. Cuando escribís RUN test.txt, lo único que hace es armar un paquete con el opcode correspondiente, escribir el path como string y la prioridad como entero, y mandarlo al Kernel por el socket. No tiene lógica de negocio, es solo un traductor de comandos humanos a paquetes de red.

Lo interesante es que la Consola es totalmente desacoplada del resto. Podés cerrarla y abrirla de nuevo sin afectar al Kernel ni a los procesos que están corriendo."

**Cierre:**
"Ahora que sabemos cómo enviarle órdenes al sistema, en el próximo video vamos a ver qué pasa dentro del Kernel cuando llega un comando RUN. Vamos a conocer la estructura de datos fundamental: el PCB o Process Control Block."

---

## Video 4: El PCB - Bloque de Control de Proceso (10-15 min)

**Introducción:**
"Bienvenidos al video 4. Hoy hablamos del PCB, el Process Control Block, que es el corazón de la gestión de procesos."

**Desarrollo:**
"El PCB es como el expediente completo de un proceso. Contiene TODA la información que el Sistema Operativo necesita para gestionar ese proceso. Si el PCB se pierde, el SO literalmente no sabe qué hacer con ese proceso.

¿Qué información contiene? Primero, identificación: el PID que es un número único y el path del archivo de instrucciones. Segundo, el estado actual: NEW, READY, EXEC, BLOCKED o EXIT. Tercero, el contexto de la CPU: todos los registros y el Program Counter que indica la próxima instrucción. Cuarto, información de planificación como prioridad, quantum asignado, quantum restante para VRR, tiempos de ejecución y espera. Quinto, gestión de memoria con la tabla de segmentos o páginas y el tamaño del proceso. Sexto, una tabla de archivos abiertos. Y séptimo, una lista de recursos adquiridos que es fundamental para la detección de deadlock, ya veremos por qué.

Hablemos de los estados. NEW es cuando el proceso acaba de ser creado pero todavía no se cargó en memoria. READY significa que está listo para ejecutar, esperando que le toque la CPU. EXEC es cuando está ejecutando activamente en la CPU, solo uno puede estar en EXEC a la vez en un sistema monocore. BLOCKED es cuando espera un evento externo como I/O. Y EXIT es cuando ya terminó, sus recursos se están liberando.

Las transiciones entre estados son: NEW a READY cuando el planificador de largo plazo lo admite y se carga en memoria. READY a EXEC cuando el planificador de corto plazo lo selecciona. EXEC a READY por desalojo de quantum. EXEC a BLOCKED cuando pide I/O o hace WAIT de un recurso. BLOCKED a READY cuando el evento se completa. Y EXEC a EXIT cuando termina normalmente o es matado.

El PCB tiene que ser thread-safe porque varios hilos del Kernel pueden acceder simultáneamente a las colas donde viven los PCBs. Por eso tenemos mutex por cola: mutex_ready, mutex_exec, mutex_blocked. Cada operación sobre una cola lockea su mutex, opera, y lo libera."

**Cierre:**
"En el próximo video vamos a ver el ciclo completo de un proceso, desde que se crea hasta que se destruye, incluyendo la interacción con el módulo de Memoria."

---

## Video 5: Creación y Destrucción de Procesos (10-15 min)

**Introducción:**
"Video 5, vamos a seguir el ciclo de vida completo de un proceso, desde su nacimiento hasta su muerte."

**Desarrollo:**
"Todo empieza cuando el usuario ejecuta RUN test.txt en la Consola. El paquete llega al Kernel, que primero valida que el archivo exista en la carpeta de procesos. Si existe, crea un PCB asignándole un PID único con un contador autoincremental.

El PCB se inicializa con estado NEW, Program Counter en 0, todos los registros en cero, y se encola en la cola de procesos nuevos. Acá es clave: el proceso está creado pero NO está en memoria todavía. Luego el Kernel hace post de un semáforo para notificar al planificador de largo plazo.

El planificador de largo plazo despierta, saca el proceso de la cola NEW, y le pide a Memoria que lo cargue. Memoria abre el archivo de instrucciones, lee línea por línea guardando cada instrucción en una lista, crea la tabla de páginas o segmento según el esquema configurado, y confirma al Kernel.

Una vez que Memoria confirmó, el Kernel mueve el proceso de NEW a READY y lo encola para que el planificador de corto plazo eventualmente lo ejecute.

Ahora, la destrucción. Hay tres formas de terminar: normal con la instrucción EXIT, forzada con KILL desde la Consola, o por error como un segmentation fault. En cualquier caso, lo primero es cambiar el estado a EXIT.

Luego viene la limpieza: liberar todos los recursos adquiridos haciendo SIGNAL implícito de cada uno, notificar a Memoria para que libere todos los frames o el segmento, invalidar las entradas de TLB si están en CPU, destruir el PCB, e incrementar el semáforo de multiprogramación para que otro proceso pueda entrar.

Esa liberación de recursos es clave. Usamos una función recursos_liberar_adquiridos que recorre la lista recursos_adquiridos del PCB. Para cada recurso, si hay alguien bloqueado esperándolo, se lo transfiere directamente a ese proceso. Si no hay nadie esperando, incrementa el contador de instancias. Esto se hace tanto en el EXIT normal como en el KILL forzoso. Si te olvidás de esto, los recursos quedan trabados para siempre y podés generar deadlocks fantasma.

Es crítico que la limpieza sea completa y atómica. Si te olvidás de liberar algo, generás leaks de recursos que eventualmente van a colapsar el sistema."

**Cierre:**
"En el próximo video vamos a profundizar en el planificador de largo plazo, viendo cómo funciona el control de admisión y el grado de multiprogramación."

---

## Video 6: Planificador de Largo Plazo (10-15 min)

**Introducción:**
"Video 6, hablamos del planificador de largo plazo. Este es el que decide CUÁLES procesos entran al sistema."

**Desarrollo:**
"El concepto clave acá es el grado de multiprogramación. Es la cantidad máxima de procesos que pueden estar en memoria simultáneamente. Si tenés GM=3, solo pueden haber 3 procesos entre READY, EXEC y BLOCKED. El cuarto va a tener que esperar en NEW.

¿Por qué limitarlo? Porque más procesos no siempre es mejor. Si tenés 100 procesos en memoria pero solo 1 CPU, vas a estar cambiando de contexto permanentemente sin hacer trabajo útil. Es overhead puro. El GM óptimo depende de cuánta memoria tenés y cuánto I/O hacen los procesos.

El planificador de largo plazo es un hilo separado que hace esto: primero espera en un semáforo que indica que hay procesos en NEW. Segundo, verifica si la planificación no está pausada con PAUSE. Tercero, espera en el semáforo de multiprogramación que actúa como contador de slots disponibles. Cuarto, saca un proceso de NEW. Quinto, le pide a Memoria que lo cargue. Y sexto, si todo OK, lo mueve a READY.

El semáforo de multiprogramación es la magia acá. Se inicializa con el valor del GM. Cada vez que un proceso es admitido, se hace wait y baja el contador. Cuando un proceso termina, se hace post y sube. Si el contador llega a cero, el planificador se bloquea hasta que termine algún proceso.

PAUSE y START son comandos útiles para debugging. PAUSE bloquea el planificador usando una condition variable. Podés crear un montón de procesos con RUN, verlos en estado NEW con PS, y luego hacer START para que empiecen a ejecutar todos juntos. Es control granular.

Si Memoria rechaza la creación porque no hay espacio, el planificador devuelve el slot de multiprogramación y destruye el PCB. Es rollback."

**Cierre:**
"El próximo video es el planificador de corto plazo, que es el más complejo y el que realmente implementa los algoritmos de scheduling que todos estudiamos en teoría."

---

## Video 7: Planificador de Corto Plazo (15-20 min)

**Introducción:**
"Video 7, el planificador de corto plazo. Este es el cerebro del scheduling."

**Desarrollo:**
"El short-term scheduler decide QUIÉN ejecuta y CUÁNDO. Corre a gran frecuencia, en el orden de milisegundos. Sus objetivos son contradictorios: maximizar throughput que es procesos por segundo, minimizar latencia que es tiempo de respuesta, y asegurar fairness que todos progresen equitativamente.

El planificador es otro hilo que hace este loop: espera en el semáforo de READY, llama a la función proximoAEjecutar que es un function pointer al algoritmo activo, marca el PCB como EXEC y guarda su PID globalmente, si el algoritmo es preemptivo lanza un hilo timer, despacha el proceso a la CPU, y cuando la CPU devuelve el control, atiende el resultado.

El dispatch es bloqueante. El hilo del planificador se queda esperando hasta que la CPU termine o sea interrumpida. Esto es importante: mientras un proceso está en la CPU, el planificador no puede seleccionar a otro. Es estrictamente secuencial.

El timer de quantum es un hilo separado que duerme por los milisegundos del quantum usando usleep, despierta, verifica si el proceso sigue en EXEC, y si está, envía una interrupción QUANTUM a la CPU por el socket de interrupciones.

La función proximoAEjecutar es un puntero a función. Permite cambiar el algoritmo de scheduling en runtime sin recompilar. Al inicio apunta al algoritmo configurado en el .config, pero con el comando ALGORITMO RR se cambia a algoritmo_obtener_rr. Tenemos 5 algoritmos: FIFO, RR, VRR, HRRN y PRIORIDAD.

Cuando la CPU devuelve el proceso, viene con un motivo: EXIT si terminó, QUANTUM si fue desalojado, IO si pidió entrada/salida, WAIT o SIGNAL si hizo sincronización. Según el motivo, el planificador toma acciones diferentes. Por ejemplo, si es QUANTUM, vuelve a encolar en READY al final. Si es IO, lo bloquea y encola en la cola del dispositivo."

**Cierre:**
"Ahora que sabemos cómo funciona el planificador, en el próximo video vamos a ver los algoritmos en detalle, empezando con FIFO y Round Robin."

---

## Video 8: FIFO y Round Robin (15-20 min)

**Introducción:**
"Video 8, algoritmos de planificación. Vamos a comparar FIFO y Round Robin."

**Desarrollo:**
"FIFO, también llamado FCFS First-Come-First-Served, es el más simple. El primero que llega es el primero que se atiende. No es preemptivo, o sea que una vez que un proceso empieza a ejecutar, solo para si hace I/O, si hace WAIT y se bloquea, o si termina. El quantum no aplica acá.

El problema de FIFO es el efecto convoy. Si un proceso largo llega primero y detrás hay 10 procesos cortos, todos esos procesos cortos van a esperar innecesariamente. La latencia promedio se dispara.

Round Robin es FIFO con tiempo compartido. Cada proceso recibe un quantum de CPU, por ejemplo 100 milisegundos. Si no termina en ese tiempo, es desalojado y va al final de la cola. Esto garantiza que todos progresan.

El quantum es el parámetro crítico. Si es muy corto, por ejemplo 1ms, vas a estar constantemente cambiando de contexto sin hacer trabajo útil. El overhead se come el CPU. Si es muy largo, por ejemplo 10 segundos, degrada a FIFO porque los procesos terminan antes de que expire.

Una regla empírica: el quantum debería ser mucho mayor que el tiempo de context switch. Si el context switch tarda 1ms, un quantum de 100ms da 1% de overhead. Para sistemas interactivos tipo desktop, 10-20ms es típico. Para servers batch, 100ms o más.

Round Robin es justo. En el peor caso, si tenés N procesos y quantum Q, sabés que tu proceso va a ejecutar cada N*Q milisegundos como máximo. Es predecible.

Veamos métricas. El waiting time con FIFO puede ser enorme para procesos que llegan tarde. Con RR, el waiting time promedio baja drásticamente. Pero RR hace más context switches, entonces el overhead es mayor. Es un trade-off."

**Cierre:**
"En el próximo video vamos a ver algoritmos más sofisticados: Virtual Round Robin que premia procesos con I/O, HRRN que combina SJF con aging, y Prioridades."

---

## Video 9: VRR, HRRN y Prioridades (15-20 min)

**Introducción:**
"Video 9, algoritmos avanzados. Vamos a ver cómo mejorar sobre Round Robin con VRR, HRRN y Prioridades."

**Desarrollo:**
"Virtual Round Robin es una extensión de RR que reconoce que no todos los procesos son iguales. Los procesos que hacen mucho I/O tienden a ser interactivos, como editores de texto o navegadores. Los que hacen puro CPU tienden a ser batch, como compilar o comprimir.

VRR premia a los procesos que vienen de I/O dándoles un quantum bonus. Entonces si el quantum normal es 100ms y el bonus es 50ms, un proceso que vuelve de I/O recibe 150ms. La idea es que esos procesos interactivos terminen rápido y vuelvan a bloquearse en I/O, mejorando la percepción de responsiveness.

HRRN es Highest Response Ratio Next. Calcula un ratio para cada proceso: (tiempo_espera + tiempo_servicio) / tiempo_servicio. El proceso con mayor ratio ejecuta. Esto combina lo mejor de SJF (Shortest Job First) con aging para evitar starvation. Un proceso largo eventualmente tendrá un ratio alto por haber esperado mucho.

Prioridades es directo: cada proceso tiene un número de prioridad, menor es más prioritario. Cuando creás un proceso con 'RUN test.txt 1', le asignás prioridad 1. Si creás otro con 'RUN test2.txt 10', el de prioridad 1 ejecuta primero. Siempre ejecuta el proceso de mayor prioridad disponible. No es preemptivo en nuestra implementación, es decir, una vez que un proceso está en CPU no se lo saca por otro de mayor prioridad.

El problema de prioridades es starvation: un proceso de baja prioridad puede nunca ejecutar si constantemente llegan procesos de alta prioridad. La solución teórica es aging: cada cierto tiempo, incrementar la prioridad de procesos que llevan mucho esperando. En nuestro TP no implementamos aging, pero es una extensión interesante que podrían agregar.

Para cambiar el algoritmo dinámicamente, usamos function pointers. La variable proximoAEjecutar apunta a la función correspondiente. Con el comando ALGORITMO VRR, simplemente cambiamos el puntero. La próxima vez que el planificador seleccione un proceso, usará el nuevo algoritmo. Es polimorfismo en tiempo de ejecución."

**Cierre:**
"En el próximo video vamos a ver qué pasa durante un cambio de contexto, el momento donde se guarda todo el estado de un proceso y se restaura el de otro."

---

## Video 10: Cambio de Contexto (10 min)

**Introducción:**
"Video 10, el cambio de contexto o context switch. Este es el overhead fundamental de la multiprogramación."

**Desarrollo:**
"Un context switch es guardar completamente el estado del proceso que está ejecutando y restaurar el estado del proceso que va a ejecutar. El estado incluye todos los registros, el Program Counter, flags, todo.

El costo típico de un context switch en hardware real es 1 a 10 microsegundos dependiendo de la arquitectura. Parece poco, pero si hacés mil context switches por segundo, perdés hasta 1% de CPU en overhead puro.

En nuestro TP, el context switch es más costoso porque involucra serializar el contexto, enviarlo por red, y deserializarlo del otro lado. Pero el concepto es el mismo.

Cuando el Kernel despacha un proceso, serializa el PID, el PC, y todos los registros en un paquete y lo envía a la CPU. La CPU deserializa y carga esos valores en su estructura de contexto local. Cuando termina o es interrumpida, la CPU serializa el contexto actualizado y lo devuelve.

Lo crítico es que el cambio debe ser atómico desde el punto de vista del proceso. El proceso no puede darse cuenta de que fue suspendido. Cuando se reanuda, debe continuar exactamente donde quedó, como si nada hubiera pasado. Por eso es vital guardar TODOS los registros.

Hay otra consecuencia: al cambiar de proceso, la cache de CPU se invalida parcialmente. El nuevo proceso va a traer sus datos a cache, desplazando los del proceso anterior. Esto se llama cache pollution y es otro costo oculto del context switch.

En sistemas multicore, las cosas se complican aún más con cache coherence, pero eso está fuera del scope de nuestro TP monocore."

**Cierre:**
"Ahora que entendemos scheduling y context switch, en el próximo video vamos a meternos en la CPU, viendo el ciclo fundamental de un procesador: Fetch-Decode-Execute."

---

## Video 11: Ciclo Fetch-Decode-Execute (15-20 min)

**Introducción:**
"Video 11, entramos en el corazón de la CPU. Vamos a ver cómo ejecuta instrucciones con el ciclo Fetch-Decode-Execute."

**Desarrollo:**
"Este ciclo es la esencia de cualquier procesador, desde tu celular hasta un supercomputador. Tiene tres fases: Fetch que trae la instrucción de memoria, Decode que interpreta qué hacer, y Execute que realiza la operación. Y esto se repite en loop hasta que el proceso termina.

El Program Counter o PC es clave. Es un registro especial que siempre apunta a la próxima instrucción a ejecutar. Empieza en 0 y se va incrementando automáticamente. Algunas instrucciones como saltos pueden modificarlo directamente.

En la fase de Fetch, la CPU le pide a Memoria la instrucción en la posición PC para el proceso actual. Memoria busca en su lista de instrucciones y devuelve un string, por ejemplo 'SET AX 10'. Esta comunicación es por socket y es bloqueante.

En Decode, la CPU parsea ese string. Lo parte por espacios, el primer token es el nombre de la instrucción, los siguientes son parámetros. Arma una estructura con el tipo de instrucción y sus operandos. Esto es análogo a lo que hace un CPU real con los bits del opcode.

En Execute, según el tipo de instrucción, se ejecuta la lógica correspondiente. SET escribe en un registro. SUM lee dos registros, los suma, y escribe el resultado. MOV_IN trae datos de memoria a un registro. IO_GEN_SLEEP bloquea el proceso.

Después del Execute, si la instrucción no modificó el PC explícitamente, se incrementa en 1. Luego se chequea si hay que terminar el ciclo. Razones para terminar: la instrucción EXIT, una interrupción pendiente como QUANTUM, o un error como SEGFAULT.

Antes de cada Fetch, hay un chequeo de interrupciones. Esto es crucial para la preemption. Si el Kernel envió una interrupción QUANTUM, el flag se setea, la CPU lo detecta, y termina el ciclo devolviendo el control aunque el proceso no haya terminado."

**Cierre:**
"En el próximo video vamos a profundizar en los registros del procesador y ver la tabla completa de instrucciones que soportamos, incluyendo las especiales WAIT y SIGNAL."

---

## Video 12: Registros e Instrucciones (10-15 min)

**Introducción:**
"Video 12, registros e Instruction Set Architecture. Vamos a ver qué registros tenemos y qué instrucciones puede ejecutar nuestra CPU."

**Desarrollo:**
"Los registros son el almacenamiento más rápido en la jerarquía de memoria. Están directamente en la CPU, el acceso es instantáneo, pero son muy pocos. Nuestra CPU tiene registros de 8 bits: AX, BX, CX, DX. Registros de 32 bits: EAX, EBX, ECX, EDX, SI, DI. Y el Program Counter que también es de 32 bits.

¿Para qué tantos tamaños? Los de 8 bits son para datos pequeños como caracteres, ahorran espacio. Los de 32 bits son para enteros, punteros, direcciones de memoria. En un CPU real como x86, hay compatibilidad legacy: AL es los 8 bits bajos de AX, que es los 16 bits bajos de EAX.

Tenemos instrucciones aritméticas: SET asigna un valor literal a un registro, SUM suma dos registros, SUB resta. Control de flujo: JNZ salta si un registro no es cero, útil para loops. Memoria: MOV_IN lee de memoria a registro, MOV_OUT escribe de registro a memoria, RESIZE cambia el tamaño del proceso.

I/O: IO_GEN_SLEEP duerme milisegundos, IO_STDIN_READ lee del teclado, IO_STDOUT_WRITE escribe en pantalla, IO_FS_CREATE/DELETE/READ/WRITE operan sobre el filesystem. Sincronización: WAIT adquiere un recurso, SIGNAL lo libera.

WAIT y SIGNAL son especiales porque la CPU no los resuelve. Cuando encuentra WAIT RA, simplemente devuelve el control al Kernel con motivo WAIT y el nombre del recurso. El Kernel es quien decide si bloquea el proceso o no. Esto es delegación de responsabilidades. La CPU ejecuta, el Kernel coordina.

Nuestra ISA es custom y simplificada, pero tiene todos los elementos conceptuales de una ISA real: cómputo, memoria, control de flujo, I/O, sincronización. Es didácticamente completa."

**Cierre:**
"En el próximo video vamos a ver cómo funciona el mecanismo de interrupciones, que es clave para el desalojo de procesos y el manejo de eventos asíncronos."

---

## Video 13: Interrupciones y Desalojo (10-15 min)

**Introducción:**
"Video 13, interrupciones. Este es el mecanismo que permite al Sistema Operativo retomar el control de la CPU."

**Desarrollo:**
"Una interrupción es una señal que rompe el flujo normal de ejecución. En hardware real, las interrupciones pueden venir del timer, del teclado, de la red, del disco. En nuestro TP, las interrupciones vienen del Kernel y van a la CPU por un socket separado.

Tenemos un flag compartido entre hilos llamado hay_interrupcion. Es volatile para evitar que el compilador lo cachee en un registro. Hay un hilo separado en la CPU escuchando en el socket de interrupciones. Cuando recibe un mensaje del Kernel con INTERRUPCION_QUANTUM, setea el flag a true y guarda el tipo.

El ciclo de instrucción chequea este flag ANTES de cada Fetch. Si está en true, inmediatamente termina el ciclo y devuelve el control al Kernel con el motivo correspondiente. No completa la instrucción actual ni nada, es inmediato.

¿Por qué chequear antes del Fetch y no durante Execute? Porque algunas instrucciones pueden tardar mucho o bloquearse. Si esperás a que termine la instrucción, perdés control. Chequeando antes del Fetch, garantizás un tiempo de respuesta acotado.

Los motivos de desalojo son: QUANTUM cuando el timer expira, KILL cuando el usuario ejecuta KILL, EXIT cuando el proceso finaliza normalmente, IO cuando el proceso pide entrada/salida, WAIT y SIGNAL, y SEGFAULT cuando hay un acceso inválido a memoria.

Cada motivo tiene una acción en el Kernel. QUANTUM reencola en READY. EXIT libera todo. IO bloquea y notifica al dispositivo. SEGFAULT puede loguear el error y matar el proceso.

El socket de interrupt está separado del socket de dispatch porque son canales de comunicación independientes. Dispatch es sincrónico y bloqueante. Interrupt es asincrónico y se procesa en un hilo separado. Esto simula cómo en hardware real las interrupciones llegan por líneas dedicadas."

**Cierre:**
"Hasta acá vimos CPU y Kernel. Ahora vamos a meternos en Memoria, empezando con los conceptos de memoria virtual y la arquitectura del módulo."

---

## Video 14: Memoria - Conceptos y Arquitectura (10-15 min)

**Introducción:**
"Video 14, entramos en gestión de memoria. Esto es uno de los subsistemas más complejos e importantes del SO."

**Desarrollo:**
"¿Por qué necesitamos abstraer la memoria? Tres razones. Primero, simplificar la programación: cada proceso cree que tiene toda la memoria para sí, empezando en dirección 0. No tiene que preocuparse dónde está físicamente. Segundo, protección: un proceso no puede leer ni escribir la memoria de otro, el SO lo previene. Tercero, flexibilidad: podés tener más memoria virtual que RAM física usando swap.

Hay dos espacios de direcciones: el lógico que ve el proceso, y el físico que es la RAM real. La traducción lógico→físico la hace un componente llamado MMU, Memory Management Unit.

Nuestro módulo Memoria mantiene dos tipos de datos: las instrucciones de cada proceso, y los datos que esos procesos escriben con MOV_OUT. Las instrucciones están en una lista cargada del archivo. Los datos están en un array de bytes que simula la RAM.

La RAM se inicializa con malloc de N bytes según la configuración. Es un bloque contiguo de memoria que representa toda la RAM física. Luego se divide en frames de tamaño fijo.

Tenemos una capa de abstracción: el esquema de memoria. Puede ser PAGINACION o SEGMENTACION, configurado en el archivo. Esta capa expone function pointers: traducir, crear_proceso, destruir_proceso, resize. Según el esquema, apuntan a funciones diferentes. Es polimorfismo.

Cuando la CPU pide leer dirección lógica 100, Memoria llama a esquema->traducir para obtener la dirección física, y luego lee de la RAM en esa posición. La CPU no sabe si está usando paginación o segmentación, es transparente."

**Cierre:**
"En el próximo video vamos a profundizar en paginación, viendo cómo funciona la tabla de páginas y la traducción de direcciones paso a paso."

---

## Video 15: Paginación (20-25 min)

**Introducción:**
"Video 15, paginación en detalle. Este es el esquema de memoria más usado en SOs modernos."

**Desarrollo:**
"Paginación divide el espacio de direcciones en bloques de tamaño fijo llamados páginas. La RAM se divide en bloques del mismo tamaño llamados frames. Típicamente 4KB en Linux, en nuestro TP es configurable.

La gran ventaja de paginación es que elimina la fragmentación externa. Como todas las unidades son del mismo tamaño, cualquier frame libre puede usarse para cualquier página. No quedan huecos inutilizables entre procesos.

La desventaja es fragmentación interna: si un proceso necesita 1KB pero las páginas son de 4KB, desperdicías 3KB en la última página.

La traducción funciona así: tomás la dirección lógica y la dividís por el tamaño de página. El cociente es el número de página, el resto es el offset dentro de esa página. Por ejemplo, si las páginas son de 256 bytes y querés acceder a la dirección 1000, página = 1000/256 = 3, offset = 1000%256 = 232.

Luego buscás en la tabla de páginas del proceso qué frame tiene asignada la página 3. Supongamos que es el frame 10. La dirección física es frame * tamaño + offset = 10 * 256 + 232 = 2792.

La tabla de páginas es una lista de entradas, una por cada página del proceso. Cada entrada tiene el número de frame asignado, y metadatos: bit de presente que indica si está en RAM o en swap, bit de dirty que indica si fue modificada, bit de use para el algoritmo de reemplazo Clock.

Cuando un proceso hace RESIZE para agrandar, Memoria calcula cuántas páginas más necesita, busca frames libres en el bitmap, crea entradas nuevas en la tabla, y marca esos frames como ocupados. Para achicar, libera las páginas del final.

El bitmap de frames es un bitarray donde cada bit representa un frame. 0 es libre, 1 es ocupado. Es eficiente en espacio: 1 bit por frame. Para buscar un frame libre, iterás hasta encontrar un 0.

Si pedís un frame y no hay libres, invocás el algoritmo de reemplazo para victimizar un frame ocupado, desalojar su contenido, y reutilizarlo."

**Cierre:**
"En el próximo video vamos a ver cómo la CPU acelera las traducciones usando una cache llamada TLB, y cómo funciona la MMU."

---

## Video 16: MMU y TLB en la CPU (15-20 min)

**Introducción:**
"Video 16, la MMU y la TLB. Estos son mecanismos para acelerar el acceso a memoria."

**Desarrollo:**
"La MMU es el hardware que traduce direcciones. Intercepta todos los accesos a memoria que hace la CPU y los traduce de lógico a físico. En nuestro TP, es un módulo de software en la CPU.

El problema es que traducir sin cache requiere consultar la tabla de páginas para cada acceso. Si la tabla está en Memoria (otro proceso), es un round-trip de red. Es lentísimo. Imaginate hacer eso para cada MOV_IN.

La solución es la TLB: Translation Lookaside Buffer. Es una cache de traducciones recientes. Guarda pares (página, frame). Cuando la CPU quiere traducir una dirección, primero busca en la TLB. Si encuentra la página, usa el frame directamente sin ir a Memoria. Eso es un TLB hit.

Si no está en la TLB, es un TLB miss. Ahí sí consulta a Memoria, obtiene el frame, lo agrega a la TLB para la próxima, y continúa. La próxima vez que acceda a la misma página, será un hit.

El hit ratio de la TLB en programas reales es típicamente 90-99%. Es alta por localidad espacial: los programas tienden a acceder direcciones cercanas en un corto tiempo, que probablemente están en la misma página.

La TLB es pequeña, por ejemplo 16 entradas en nuestro TP. Cuando se llena, hay que reemplazar. Tenemos dos algoritmos: FIFO que reemplaza la entrada más antigua, y LRU que reemplaza la menos usada recientemente. LRU es mejor pero requiere mantener timestamps.

Cuando hay un context switch, la TLB se invalida para el proceso saliente. Si no lo hacés, el nuevo proceso podría usar traducciones del proceso anterior, accediendo memoria ajena. Esto es un bug de seguridad grave. Entonces al cambiar de proceso, la CPU limpia las entradas de TLB de ese PID."

**Cierre:**
"En el próximo video vamos a ver qué pasa cuando la RAM se llena: algoritmo de reemplazo de páginas Clock y swap."

---

## Video 17: Reemplazo de Páginas y Swap (15-20 min)

**Introducción:**
"Video 17, reemplazo y swap. ¿Qué hace el SO cuando se acaba la RAM?"

**Desarrollo:**
"Cuando un proceso trata de acceder a una página que no está en RAM, el bit de presente es false, eso es un page fault. El SO debe traer esa página desde swap. Pero si no hay frames libres, primero debe desalojar una página víctima.

El algoritmo de reemplazo decide qué página sacar. Idealmente sacamos la que no se va a usar en un futuro cercano, pero eso requiere conocer el futuro. Algoritmos prácticos aproximan eso con heurísticas.

Clock es una aproximación de LRU. Mantiene un puntero circular que recorre los frames. Cada entrada de tabla de páginas tiene un bit de use. La CPU setea este bit cada vez que accede a la página.

Clock funciona así: el puntero avanza buscando un frame con use=0. Si encuentra uno con use=1, le da una segunda oportunidad: setea use=0 y sigue. Si encuentra use=0, esa es la víctima. Es justo porque todos los frames tienen la misma probabilidad de ser chequeados.

Antes de reemplazar, se chequea el bit dirty. Si es true, la página fue modificada y hay que escribirla a swap para no perder datos. Si es false, simplemente se descarta porque hay una copia idéntica en swap o es una página de solo lectura.

El swap es un archivo en disco, por ejemplo proceso_1.swap. Se divide en bloques del tamaño de página. Para escribir la página 3 del proceso 1, se hace fseek a offset 3*tamaño_pagina y fwrite.

Traer de swap es lo inverso: fseek y fread. Es lento comparado con RAM, típicamente 1000x más. Por eso hacer muchos page faults (thrashing) mata el performance.

En sistemas reales, cuando el thrashing es severo, el SO mata procesos para liberar memoria. Es drástico pero necesario para mantener el sistema funcional."

**Cierre:**
"En el próximo video vamos a ver segmentación como alternativa a paginación, entendiendo sus pros y contras."

---

## Video 18: Segmentación (15 min)

**Introducción:**
"Video 18, segmentación. Un enfoque diferente a la gestión de memoria."

**Desarrollo:**
"En segmentación, cada proceso recibe un bloque contiguo de memoria de tamaño variable. Se registra la base, que es la dirección de inicio, y el límite, que es el tamaño. La traducción es trivial: dirección_física = base + offset. Si offset >= límite, segmentation fault.

La ventaja es simplicidad. No hay tabla de páginas, no hay fragmentación interna. El proceso puede crecer hasta su límite sin desperdiciar espacio.

La desventaja fatal es fragmentación externa. Supongamos que tenés procesos de 100KB, 200KB, 50KB en memoria. Luego el de 200KB termina, dejando un hueco. Llega un proceso de 300KB, no cabe en el hueco aunque hay 200KB libres. Se desperdicia.

La solución clásica es compactación: mover todos los procesos para que estén contiguos, juntando los huecos en un solo bloque libre al final. Pero esto es lentísimo, requiere copiar gigas de memoria, y mientras tanto todos los procesos están pausados.

Por esta razón, los SOs modernos no usan segmentación pura. Linux usa paginación. Pero x86 en modo protegido tiene segmentación por legacy, combinada con paginación.

En nuestro TP, implementamos segmentación para fines didácticos. Usamos una free list que rastrea los huecos libres. El algoritmo First-Fit busca el primer hueco que sea suficientemente grande. Best-Fit busca el más ajustado. Worst-Fit busca el más grande. Cada uno tiene trade-offs.

Cuando un segmento se libera, chequeamos si es adyacente a huecos existentes y los mergeamos. Así evitamos que la free list se fragmente en muchos huequitos chicos."

**Cierre:**
"Ahora cambiamos de tema. Los próximos tres videos son sobre sincronización y deadlock. Empezamos con WAIT y SIGNAL."

---

## Video 19: Recursos: WAIT y SIGNAL (15-20 min)

**Introducción:**
"Video 19, sincronización con semáforos. WAIT y SIGNAL son las primitivas para coordinar procesos."

**Desarrollo:**
"Un semáforo es un contador que controla el acceso a un recurso compartido. Cada recurso tiene instancias. Por ejemplo, si tenés 3 impresoras, el semáforo empieza en 3.

WAIT intenta adquirir una instancia. Si hay disponibles (contador > 0), decrementa el contador y continúa. Si no hay (contador = 0), bloquea el proceso encolándolo.

SIGNAL libera una instancia. Si hay procesos bloqueados esperando, desbloquea al primero de la cola dándole el recurso directamente. Si no hay bloqueados, incrementa el contador.

Es crítico que estas operaciones sean atómicas. Si dos procesos hacen WAIT simultáneamente en un semáforo con 1 instancia, uno debe bloquearse. Si no usás mutex, ambos pueden ver contador=1, pasar el chequeo, y adquirir, violando la exclusión mutua.

En nuestro Kernel, cada recurso tiene su propio mutex que protege el contador y la cola de bloqueados. WAIT hace mutex_lock al iniciar y mutex_unlock al salir.

El tracking es importante. El PCB tiene una lista de recursos_adquiridos. Cuando un proceso adquiere RA, se agrega "RA" a esta lista. Cuando hace SIGNAL o cuando termina, se remueve. Esto permite liberar automáticamente todos los recursos al finalizar, evitando deadlocks por procesos muertos.

Los recursos se configuran en el kernel.config: RECURSOS=[RA,RB,RC] e INSTANCIAS_RECURSOS=[1,2,1]. Esto significa 1 instancia de RA, 2 de RB, 1 de RC.

En sistemas reales, los semáforos se usan para todo: archivos, sockets de red, límites de threads, conexiones de DB. Son la herramienta fundamental de sincronización."

**Cierre:**
"En el próximo video vamos a ver el lado oscuro de la sincronización: el deadlock. Vamos a detectarlo construyendo un grafo de espera."

---

## Video 20: Detección de Deadlock con Grafo de Espera (15-20 min)

**Introducción:**
"Video 20, deadlock. El problema clásico de sincronización."

**Desarrollo:**
"Un deadlock es cuando dos o más procesos se esperan mutuamente formando un ciclo, y ninguno puede progresar. El ejemplo canónico: proceso A tiene RA y espera RB, proceso B tiene RB y espera RA. Deadlock.

Las cuatro condiciones necesarias de Coffman son: exclusión mutua, los recursos no se comparten. Hold and wait, procesos pueden tener recursos y pedir más. No preemption, no se les quita recursos forzosamente. Espera circular, existe un ciclo de procesos esperándose.

Para que haya deadlock, las cuatro condiciones deben cumplirse. Romper cualquiera lo previene. Pero algunas son imposibles de romper sin cambiar el problema. Por ejemplo, exclusión mutua es inherente a ciertos recursos como impresoras.

El grafo de espera es un grafo dirigido donde los nodos son procesos y hay una arista del proceso A al proceso B si A espera un recurso que tiene B. Detectar deadlock es detectar un ciclo en este grafo.

Para construir el grafo, iteramos sobre cada recurso. Para cada proceso bloqueado esperando ese recurso, buscamos quién tiene instancias de ese recurso adquiridas. Dibujamos una arista del bloqueado al dueño.

Para detectar ciclos, usamos DFS con un conjunto en_stack que rastrea el camino actual. Si durante el DFS encontramos un nodo que ya está en_stack, hay ciclo. Reconstruimos el camino para loguear el ciclo exacto.

¿Cuándo ejecutar la detección? Después de cada WAIT que bloquea, porque es cuando el grafo cambia. También podés ejecutarlo periódicamente cada N segundos como un hilo de fondo.

¿Qué hacer si detectás deadlock? Opciones: matar uno de los procesos del ciclo, forzar SIGNAL de uno de los recursos, rollback. Todas tienen consecuencias negativas. Lo ideal es prevenir, no detectar."

**Cierre:**
"En el próximo video vamos a ver el algoritmo del Banquero, que previene deadlock rechazando pedidos que llevan a estados inseguros."

---

## Video 21: Algoritmo del Banquero (15-20 min)

**Introducción:**
"Video 21, el algoritmo del Banquero de Dijkstra. Prevención de deadlock."

**Desarrollo:**
"El Banquero diferencia entre detectar deadlock actual y detectar un estado inseguro que podría llevar a deadlock futuro. Es preventivo.

Tenemos tres matrices: Available es un vector con las instancias libres de cada recurso. Allocation es una matriz procesos x recursos con lo que cada proceso tiene asignado. Need es una matriz con lo que cada proceso todavía necesita para terminar.

El algoritmo simula: ¿hay algún proceso que pueda terminar con los recursos disponibles? Si su Need <= Available, ese proceso puede completar. Simulamos su terminación liberando sus recursos, sumándolos a Available. Marcamos ese proceso como terminado. Repetimos hasta que todos terminen o nos trabemos.

Si al final todos pudieron terminar, el estado es seguro. Hay al menos una secuencia de ejecución valid que evita deadlock. Si quedó alguno sin terminar, el estado es inseguro y eventualmente podría haber deadlock.

El Banquero se ejecuta antes de otorgar un recurso. Simula otorgarlo, corre el algoritmo, y si el estado resultante es seguro, lo otorga. Si es inseguro, rechaza el pedido bloqueando al proceso hasta que sea seguro.

El problema es que requiere saber de antemano cuántos recursos necesita cada proceso en total. En la práctica, esto es difícil de estimar. Por eso el Banquero es más académico que práctico.

Comparando con el grafo: el grafo detecta deadlock actual, trabaja sobre la realidad. El Banquero detecta estados inseguros, trabaja sobre lo que podría pasar. El grafo es reactivo, el Banquero es proactivo.

Linux no usa el Banquero. Usa detección y recovery matando procesos si es necesario. Es más pragmático aunque menos elegante."

**Cierre:**
"Cambiamos de tema nuevamente. Los próximos dos videos son sobre Entrada/Salida. Empezamos con I/O genérica, STDIN y STDOUT."

---

## Video 22: IO Genérica, STDIN y STDOUT (15 min)

**Introducción:**
"Video 22, entrada y salida. Cómo interactúan los procesos con el mundo exterior."

**Desarrollo:**
"Los dispositivos de I/O son inherentemente lentos comparados con la CPU. Un disco tarda milisegundos, la red decenas o cientos, el usuario puede tardar segundos en escribir algo. Si la CPU esperara activamente, desperdiciaría millones de ciclos.

La solución es blocking I/O. Cuando un proceso pide I/O, se bloquea inmediatamente. El Kernel encola la operación en el dispositivo y pone el proceso en BLOCKED. Luego el planificador selecciona otro proceso. Cuando el dispositivo termina, notifica al Kernel, y el proceso vuelve a READY.

En nuestro TP, cada dispositivo de I/O es un proceso separado del módulo EntradaSalida. Cada uno se registra en el Kernel al iniciar, indicando su nombre y tipo: GENERICA, STDIN, STDOUT, o DIALFS.

IO_GEN_SLEEP es la más simple. Recibe milisegundos, usa usleep para dormir, y cuando despierta notifica al Kernel. Simula un dispositivo que tarda cierto tiempo.

IO_STDIN_READ lee del teclado real. Usa fgets para bloquear esperando input del usuario. Los datos leídos se escriben en la Memoria del proceso en la dirección lógica especificada. Luego notifica.

IO_STDOUT_WRITE hace lo inverso. Lee de la Memoria del proceso y lo imprime con printf. Útil para debugging, ver qué valores tiene el proceso en memoria.

El flujo es: la CPU ejecuta IO_STDOUT_WRITE, devuelve al Kernel con motivo IO y los parámetros (interfaz, dirección, tamaño). El Kernel bloquea el proceso y le manda un paquete al proceso de I/O correspondiente. El I/O ejecuta la operación, que incluye comunicarse con Memoria para leer/escribir. Al terminar, el I/O notifica al Kernel. El Kernel desbloquea el proceso.

Es asíncrono de punta a punta. El proceso que pidió I/O no está activamente esperando, está bloqueado. Otros procesos ejecutan mientras tanto."

**Cierre:**
"En el próximo video vamos a ver DialFS, el filesystem completo con CREATE, DELETE, WRITE y READ de archivos."

---

## Video 23: DialFS - Filesystem (15-20 min)

**Introducción:**
"Video 23, DialFS. Vamos a implementar un filesystem simple pero funcional."

**Desarrollo:**
"Un filesystem organiza datos en archivos y directorios en disco. Necesita resolver: cómo almacenar los datos, cómo rastrear qué bloques usa cada archivo, cómo saber qué bloques están libres.

DialFS divide el disco en bloques de tamaño fijo, por ejemplo 64 bytes. Un bitmap rastrea qué bloques están ocupados. Cada archivo tiene un FCB, File Control Block, que es metadata: nombre, bloque de inicio, tamaño en bytes.

CREATE crea un archivo. Inicializa un FCB con tamaño cero y bloque_inicio indefinido por ahora. Guarda el FCB en un archivo separado, por ejemplo archivo.txt.fcb.

WRITE escribe datos en un archivo. Primero carga el FCB para saber dónde está. Calcula cuántos bloques necesita según el offset + tamaño a escribir. Si necesita más bloques de los que tiene, busca bloques libres contiguos en el bitmap, los marca como ocupados, y actualiza el FCB. Luego escribe los datos en el archivo de bloques con fseek y fwrite. Finalmente actualiza el tamaño en el FCB si creció.

READ lee datos. Carga el FCB, verifica que offset + tamaño <= tamaño_archivo, hace fseek al bloque_inicio * tamaño_bloque + offset, y fread. Luego escribe esos datos en la Memoria del proceso.

DELETE libera los bloques en el bitmap y borra el archivo .fcb.

TRUNCATE cambia el tamaño. Si achica, libera bloques del final. Si agranda, asigna más bloques.

La complejidad acá es que los bloques deben ser contiguos para simplificar. En filesystems reales como ext4, los bloques pueden estar dispersos y se usa una lista de punteros (inodos) o un árbol (B-tree).

DialFS no tiene directorios, todos los archivos están en un solo nivel. No tiene permisos. No tiene journaling para recuperación ante crashes. Es una versión ultra simplificada didáctica."

**Cierre:**
"En el próximo video vamos a ver cómo verificar que todo funciona correctamente con una suite automatizada de tests."

---

## Video 24: Test Suite - Verificación del SO (10-15 min)

**Introducción:**
"Video 24, testing. Cómo asegurarnos de que nuestro SO funciona correctamente."

**Desarrollo:**
"Testing en sistemas complejos como un SO es difícil porque hay concurrencia, timing, múltiples procesos. No podés simplemente hacer asserts como en unit tests. La estrategia acá es testing basado en logs.

Generamos logs detallados de cada acción. Luego verificamos que ciertos patrones aparecen en el orden correcto. Por ejemplo, para el Test 1 de ciclo básico, verificamos que aparezca 'Proceso creado: PID=1', luego 'Proceso PID=1 creado' en Memoria, luego 'Ejecutando EXIT' en CPU, y finalmente 'Proceso PID=1 finalizó' en Kernel.

Los scripts de test usan Bash con funciones helper. wait_for_log busca un patrón en un archivo de log con timeout. Si no aparece en N segundos, el test falla. wait_for_log_count espera que un patrón aparezca N veces, útil para verificar que algo se repitió.

Tenemos 11 tests que cubren: Test 1 ciclo básico de NEW a EXIT. Test 2 I/O bloqueante con sleep. Test 3 I/O múltiple con STDIN y STDOUT. Test 4 desalojo por quantum con Round Robin. Test 5 sincronización con recursos compartidos. Test 6 acceso a memoria con MOV_IN y MOV_OUT. Test 10 prioridades con PAUSE/START. Test 11 detección de deadlock.

Cada test tiene su archivo de instrucciones en memoria/procesos/. Por ejemplo, test1.txt tiene instrucciones simples, deadlock_a.txt y deadlock_b.txt están diseñados para generar abrazo mortal.

Para ejecutar todos los tests, corremos run_all.sh. Este script levanta todos los módulos, corre cada test verificando logs, y reporta cuántos pasaron. Si alguno falla, muestra qué patrón no se encontró.

La automatización es clave. Si no tenés tests, cada cambio requiere testeo manual que es tedioso y propenso a errores. Con la suite, corrés un script y en 5 minutos sabés si rompiste algo."

**Cierre:**
"Y llegamos al último video. Vamos a hacer una recapitulación completa del sistema, viendo cómo encajan todas las piezas."

---

## Video 25: Recapitulación - El SO Completo (10-15 min)

**Introducción:**
"Video 25, la recapitulación final. Vamos a repasar todo lo que construimos y cómo funciona de punta a punta."

**Desarrollo:**
"Empecemos con un recorrido completo. El usuario escribe RUN test.txt en la Consola. La Consola envía un paquete al Kernel. El Kernel valida el archivo, crea un PCB en estado NEW con PID 1, lo encola, y notifica al planificador de largo plazo con un semáforo.

El planificador de largo plazo despierta, verifica el grado de multiprogramación, saca el PCB de NEW, le pide a Memoria que cargue el proceso. Memoria abre test.txt, lee las instrucciones línea por línea, crea una tabla de páginas vacía o un segmento según el esquema, confirma al Kernel.

El Kernel mueve el proceso de NEW a READY, lo encola, notifica al planificador de corto plazo. Este selecciona el PCB según el algoritmo activo (FIFO, RR, VRR, HRRN, o prioridad), lo marca como EXEC, lanza un timer si es preemptivo, y despacha el contexto a la CPU por socket.

La CPU recibe el contexto, empieza el ciclo fetch-decode-execute. Hace fetch pidiendo la instrucción 0 a Memoria, recibe 'SET AX 10', decodea, ejecuta escribiendo 10 en el registro AX, incrementa el PC a 1. Antes del próximo fetch, chequea interrupciones. No hay, continúa. Fetch de instrucción 1, y así hasta EXIT.

Si el proceso hace MOV_OUT 0 AX, la CPU llama a la MMU para traducir la dirección lógica 0. La MMU busca en la TLB, si es la primera vez es miss, consulta a Memoria el número de frame, lo agrega a la TLB, calcula la dirección física, y la CPU escribe en Memoria esa dirección física con el valor de AX.

Si el proceso hace IO_STDOUT_WRITE, la CPU devuelve al Kernel con motivo IO. El Kernel bloquea el proceso, lo envía al dispositivo de I/O. El I/O lee de Memoria los datos, los imprime, notifica al Kernel. El Kernel mueve el proceso de BLOCKED a READY, el planificador eventualmente lo reselecciona, y continúa donde quedó.

Si el proceso hace WAIT RA, la CPU devuelve con motivo WAIT. El Kernel verifica instancias del recurso RA. Si hay, decrementa y continúa. Si no, bloquea el proceso en la cola del recurso. Cuando otro proceso hace SIGNAL RA, desbloquea al primero de la cola.

El ciclo termina con EXIT. La CPU devuelve al Kernel. El Kernel libera todos los recursos con SIGNAL interno, notifica a Memoria para destruir la tabla y liberar frames, destruye el PCB, incrementa el semáforo de multiprogramación.

Los patrones de diseño que usamos: semáforos para sincronización y notificación entre hilos, mutex para exclusión mutua en estructuras compartidas, condition variables para pausar y reanudar planificación, function pointers para cambiar algoritmos dinámicamente, serialización para comunicación entre procesos.

Comparado con un SO real como Linux: nosotros tenemos 1 CPU, ellos tienen múltiples cores con scheduling complejo. Nosotros no tenemos cache de RAM, ellos tienen L1, L2, L3 con coherencia. Nosotros tenemos filesystem básico, ellos tienen ext4 con journaling y millones de features. Nosotros tenemos interrupciones simuladas por sockets, ellos tienen IRQs de hardware. Pero conceptualmente, los fundamentos son los mismos.

Lo que logramos: gestión completa de procesos con 5 estados, 5 algoritmos de scheduling, memoria virtual con paginación y segmentación, MMU con TLB, swap con Clock, sincronización con semáforos, detección de deadlock con grafo y Banquero, I/O asíncrona con STDIN/STDOUT/filesystem, comunicación por sockets con protocolo bien definido, y suite de tests automatizada.

Más importante que el código es el conocimiento. Ahora entendés cómo funciona tu computadora por dentro. Cuando veas 100% de CPU en un proceso, sabés que puede estar en un loop infinito o bloqueado en I/O. Cuando veas thrashing con swapping constante, sabés que necesitás más RAM. Cuando veas deadlock en producción, sabés cómo analizar el grafo de espera.

Las habilidades que aprendiste: programación concurrente con threads, sincronización correcta con mutex y semáforos, debugging de race conditions, diseño de protocolos de red, manejo de errores y edge cases, testing automatizado.

Para profundizar: leé Operating System Concepts de Silberschatz para la teoría formal. Estudiá el código de xv6, un Unix didáctico del MIT. Empezá a leer el kernel de Linux, específicamente el scheduler y la gestión de memoria. Tomá cursos avanzados como MIT 6.828 o Stanford CS140.

Y lo más importante: experimentá. Modificá el código, agregá features, rompelo y arreglalo. Ese es el mejor aprendizaje."

**Cierre:**
"¡Felicitaciones! Completaste el curso de Sistemas Operativos. Has construido un SO funcional desde cero. Ahora tenés las herramientas para entender y trabajar con cualquier sistema operativo. ¡Mucho éxito en tu carrera y gracias por seguir el curso hasta el final!"

---

**FIN DEL SPEECH GUIDE**
