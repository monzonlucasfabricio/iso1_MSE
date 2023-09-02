# Lista de requerimientos

### 1) El sistema operativo (de aquí en más nombrado como OS) será preferentemente del tipo estático, entendiéndose por este término que la totalidad de memoria asociada a cada tarea y al kernel será definida en tiempo de compilación. Si el estudiante así lo quisiese, puede implementar un sistema operativo del tipo dinámico (esto es, estilo freeRTOS) donde se asigne la memoria asociada a cada tarea y a otros elementos de forma dinámica.
NOTA: El último párrafo no implica una funcionalidad obligatoria, sino opcional. El estudiante debe tener en cuenta que optar por esta metodología puede llevar a problemas en tiempo de ejecución de difícil depuración.

### 2) La cantidad de tareas que soportará el OS será ocho (8).

### 3) El OS debe administrar las IRQ del hardware.

### 4) El kernel debe poseer una estructura de control la cual contenga como mínimo los siguientes campos:

- a) Último error ocurrido
- b) Estado de sistema operativo, por ejemplo: Reset, corriendo normal, interrupción,
etc. NOTA: Los estados mencionados no son obligatorios sino solamente un ejemplo.
- c) Bandera que indique la necesidad de ejecutar un scheduling al salir de una IRQ.
- d) Puntero a la tarea en ejecución.
- e) Puntero a la siguiente tarea a ejecutar.

### 5) Cada tarea tendrá asociada una estructura de control que, como mínimo, tendrá los siguientes campos:

- a) Stack (array).
- b) Stack Pointer.
- c) Punto de entrada (usualmente llamado entryPoint).
- d) Estado de ejecución.
- e) Prioridad.
- f) Número de ID. (opcional)
- g) Nombre de la tarea en string (opcional).
- h) Ticks bloqueada.


### 6) Los estados de ejecución de una tarea serán los siguientes:

- a) Corriendo (Running).
- b) Lista para ejecución (Ready).
- c) Bloqueada (Blocked).
- d) Suspendida (Suspended) - Opcional

### 7) El tamaño de stack para cada tarea será de 256 bytes.

### 8) La implementación de prioridades será de 4 niveles, donde el nivel cero (0) será el de más alta prioridad y tres (3) será el nivel de menor prioridad.

### 9) La política de scheduling entre tareas de la misma prioridad será del tipo Round-Robin.

### 10) El tick del sistema será de 1 [ms].

### 11) El OS debe tener hooks definidos como funciones WEAK para la ejecución de código en las siguientes condiciones:

- a) Tick del sistema (tickHook).
- b) Ejecución de código en segundo plano (taskIdle).
- c) Error y retorno de una de las tareas (returnHook).
- d) Error del OS (errorHook).

### 12) El tamaño de stack para cada tarea será de 256 bytes.

- a) Función de retardos (delay).
- b) Semáforos binarios.
- c) Colas (queue).
- d) Secciones críticas
- e) Forzado de Scheduling (cpu yield).