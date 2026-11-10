#include "../include/minios.h"

Proceso *lista_procesos = NULL;

void proc_init() {
    signal(SIGTSTP, SIG_IGN); 
}

void proc_crear(int pid) {
    Proceso *p = (Proceso *)malloc(sizeof(Proceso));
    p->pid = pid;
    p->usuario = usuario_actual;
    p->inicio = time(NULL); 
    p->estado = 0; 
    p->sig = lista_procesos;
    lista_procesos = p;
}

void proc_listar() {
    printf("\n--- Lista de Procesos Activos ---\n");
    Proceso *p = lista_procesos;
    while (p) {
        char buffer_hora[26];
        ctime_r(&p->inicio, buffer_hora);
        buffer_hora[24] = '\0'; 
        
        printf("PID: %d | Dueño: %s | Estado: %s | Inicio: %s\n", 
               p->pid, p->usuario == ROL_ADMIN ? "Admin" : "Invitado", 
               p->estado == 0 ? "Ejecutando" : "Suspendido", buffer_hora);
        p = p->sig;
    }
}

void proc_suspender(int pid) {
    Proceso *p = lista_procesos;
    while (p) {
        if (p->pid == pid) {
            if (p->usuario != usuario_actual && usuario_actual != ROL_ADMIN) {
                printf("Error: No tienes permisos.\n");
                return;
            }
            p->estado = 1;
            printf("Proceso %d suspendido.\n", pid);
            return;
        }
        p = p->sig;
    }
    printf("Proceso no encontrado.\n");
}

void proc_reanudar(int pid) {
    Proceso *p = lista_procesos;
    while (p) {
        if (p->pid == pid) {
            if (p->usuario != usuario_actual && usuario_actual != ROL_ADMIN) {
                printf("Error: No tienes permisos.\n");
                return;
            }
            p->estado = 0;
            printf("Proceso %d reanudado.\n", pid);
            return;
        }
        p = p->sig;
    }
    printf("Proceso no encontrado.\n");
}
