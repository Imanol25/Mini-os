#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "proc.h"
#include "seguridad.h"
#include "kernel.h"

#define MAX_PROC 10
static PCB tabla_proc[MAX_PROC];
static int total_proc = 0;

static void esperar_hijo(PCB *p) {
    int st;
    waitpid(p->pid, &st, WUNTRACED);
    p->estado = WIFSTOPPED(st) ? SUSPENDIDO : TERMINADO;
}

void proc_lanzar(const char *programa) {
    if (total_proc >= MAX_PROC) return;
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGTSTP, SIG_DFL);
        signal(SIGINT,  SIG_DFL);
        execlp(programa, programa, (char *)NULL);
        perror("exec");
        _exit(127);
    }
    PCB *p = &tabla_proc[total_proc++];
    p->pid = pid;
    strncpy(p->nombre, programa, sizeof p->nombre - 1);
    strncpy(p->usuario, seg_usuario_actual(), sizeof p->usuario - 1);
    p->estado = EJECUTANDO;
    p->inicio = time(NULL);
    esperar_hijo(p);
    signal(SIGINT, SIG_DFL);
}

void proc_listar(void) {
    printf("\n--- TABLA DE PROCESOS ---\n");
    for(int i=0; i<total_proc; i++) {
        printf("PID: %d | Proc: %s | User: %s | Estado: %s\n", 
            tabla_proc[i].pid, tabla_proc[i].nombre, tabla_proc[i].usuario,
            tabla_proc[i].estado == SUSPENDIDO ? "SUSPENDIDO" : "TERMINADO");
    }
}

void proc_reanudar(void) {
    int pid = leer_entero("PID a reanudar: ");
    for(int i=0; i<total_proc; i++) {
        if(tabla_proc[i].pid == pid && tabla_proc[i].estado == SUSPENDIDO) {
            signal(SIGINT, SIG_IGN);
            kill(pid, SIGCONT);
            tabla_proc[i].estado = EJECUTANDO;
            esperar_hijo(&tabla_proc[i]);
            signal(SIGINT, SIG_DFL);
        }
    }
}

void proc_terminar(void) {
    int pid = leer_entero("PID a terminar: ");
    for(int i=0; i<total_proc; i++) {
        if(tabla_proc[i].pid == pid) {
            if(seg_obtener_rol() == ROL_OPERADOR && strcmp(tabla_proc[i].usuario, seg_usuario_actual()) != 0) {
                printf("Propiedad denegada: Solo puedes terminar tus procesos.\n"); return;
            }
            kill(pid, SIGTERM);
            kill(pid, SIGCONT);
            int st;
            waitpid(pid, &st, 0);
            tabla_proc[i].estado = TERMINADO;
            printf("Proceso finalizado.\n");
        }
    }
}
