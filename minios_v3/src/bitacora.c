#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "kernel.h"
#include "seguridad.h"
#include "bitacora.h"

#define ARCH_BITACORA DIR_DATOS "bitacora.log"
static int denegaciones_sesion = 0;

void auditar(const char *accion, const char *objeto, const char *resultado) {
    char fecha[32], linea[256]; time_t t = time(NULL);
    strftime(fecha, sizeof fecha, "%Y-%m-%d %H:%M:%S", localtime(&t));
    
    if (strcmp(resultado, "DENEGADO") == 0) {
        denegaciones_sesion++;
        if (denegaciones_sesion == 5) {
            printf("\n[ALERTA] Ha acumulado 5 accesos denegados en esta sesión.\n");
            int n_alerta = snprintf(linea, sizeof linea, "%s | %-10s | %-8s | ALERTA         | %-10s | 5 DENEGACIONES\n",
                     fecha, seg_usuario_actual(), seg_rol_actual(), "sistema");
            int fd_alerta = open(ARCH_BITACORA, O_WRONLY | O_CREAT | O_APPEND, 0600);
            if (fd_alerta >= 0) { write(fd_alerta, linea, n_alerta); close(fd_alerta); }
        }
    }
    int n = snprintf(linea, sizeof linea, "%s | %-10s | %-8s | %-14s | %-10s | %s\n",
                     fecha, seg_usuario_actual(), seg_rol_actual(), accion, objeto, resultado);
    int fd = open(ARCH_BITACORA, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd >= 0) { write(fd, linea, n); close(fd); }
}

void bitacora_mostrar(void) {
    FILE *f = fopen(ARCH_BITACORA, "r");
    if (!f) { printf("Bitácora vacía.\n"); return; }
    char linea[256]; int denegados = 0, fallidos = 0;
    printf("\n--- BITACORA DE AUDITORIA ---\n");
    while (fgets(linea, sizeof linea, f)) {
        printf("%s", linea);
        if (strstr(linea, "DENEGADO")) denegados++;
        if (strstr(linea, "LOGIN FALLIDO")) fallidos++;
    }
    fclose(f);
    printf("\nResumen: %d accesos denegados, %d logins fallidos\n", denegados, fallidos);
}
