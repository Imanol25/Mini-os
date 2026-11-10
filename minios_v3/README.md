# MiniOS v3.0

## 1. Integrantes y Compilación
- Compilar: Ejecutar `make` en la terminal (automatiza la compilación con gcc -Wall -Wextra).
- Limpiar: Ejecutar `make clean`.
- Iniciar: Ejecutar `./minios`.

## 2. Diagrama de Capas Arquitectónicas
- **Capa 4 (Interfaz):** `main.c`
- **Capa 3 (Servicios):** `proc.c`, `juego.c`, `sistema.c`
- **Capa 2 (Protección):** `seguridad.c` (Matriz de Control)
- **Capa 1 (Sistema de Archivos):** `fs.c` (Disco virtual y superbloque)
- **Capa 0 (Linux):** Llamadas al sistema POSIX (`open`, `read`, `write`, `fork`, `exec`).

## 3. Explicación de llamadas al sistema (strace)
Al hacer un Create y un Read en el disco virtual (`minios.disk`), `strace` revelará llamadas a:
- `lseek`: Ubica el cursor exactamente en el ID deseado (Acceso directo a la ranura del struct sin leer secuencialmente el archivo completo).
- `write` / `read`: Almacenamiento a bajo nivel en memoria secundaria.
- `fsync`: Obliga al SO anfitrión a vaciar los búferes y escribir los datos permanentemente en el disco físico.

## 4. Naturaleza del Sistema Operativo
Este sistema se asemeja a un Sistema de **Tiempo Compartido e Interactivo (ej. UNIX temprano / Multics)**. Implementa sesiones multicliente (Matriz de Acceso), permite enviar procesos a segundo plano (SIGTSTP) y gestiona un sistema de archivos virtual y encapsulado protegiendo la información según el usuario en sesión.

## 5. Matriz de Protección y Justificación
| Rol | Registros | Juegos | Partidas | Procesos | Matriz | Sistema |
| --- | --- | --- | --- | --- | --- | --- |
| **Admin** | CRUD | X | R | RUD | RU | X |
| **Operador** | CRU | X | CRU | RUD | R | - |
| **Invitado** | R | X | - | R | - | - |
*Principios Aplicados*:
- **Separación de Privilegios:** Solo el rol de Administrador tiene acceso al derecho EXECUTE sobre el Sistema (Apagar).
- **Mínimo Privilegio:** El invitado puede acceder a Juegos y listar procesos, pero se le restringe cualquier modificación o almacenamiento.
- **Fallo Seguro:** Si el usuario no tiene los bits requeridos en la matriz de acceso, la operación se deniega por defecto protegiendo el sistema.
## 6. Virtualización
- **Hipervisor detectado:** Tipo 2 (Alojado) como VirtualBox o VMware.
- **Implementación:** Asistida por Hardware (VT-x / AMD-V) para aceleración. 
- **Diferencia anfitrión/VM:** En la VM, comandos como grep 'vmx' varían si el hipervisor oculta la bandera de virtualización anidada. Los contenedores (Docker) comparten el Kernel, las VM virtualizan hardware completo.

## 7. Mejoras de la Versión Final (Parte 2)
- **Autoguardado con Mutex:** Previene condiciones de carrera. Sin `pthread_mutex`, el hilo de guardado podría capturar variables a medias (ej. posición X vieja y Y nueva) mientras el jugador se mueve, corrompiendo la partida guardada.
- **RAID 1 (Disco Espejo):** `fs.c` escribe bidireccionalmente. Si se elimina `minios.disk`, se reconstruye automáticamente desde `minios_espejo.disk`.
- **Monitor de Memoria:** `proc.c` extrae en tiempo real la paginación y huella de RAM (VmSize/VmRSS) de cada proceso en `/proc/<pid>/status`.
