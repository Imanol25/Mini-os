#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "kernel.h"
#include "seguridad.h"

#define ARCH_MATRIZ DIR_DATOS "matriz.dat"

static unsigned char matriz[NUM_ROLES][NUM_OBJETOS] = {
/*             registros                          juegos  partidas                   procesos                  matriz           sistema */
/* admin    */ { D_CREATE|D_READ|D_UPDATE|D_DELETE, D_EXEC, D_READ,                   D_READ|D_UPDATE|D_DELETE, D_READ|D_UPDATE, D_EXEC },
/* operador */ { D_CREATE|D_READ|D_UPDATE,          D_EXEC, D_CREATE|D_READ|D_UPDATE, D_READ|D_UPDATE|D_DELETE, D_READ,          0      },
/* invitado */ { D_READ,                            D_EXEC, 0,                        D_READ,                   0,               0      },
};

static const char *NOMBRE_ROL[NUM_ROLES] = { "admin", "operador", "invitado" };
static int rol_actual = -1;

void seg_iniciar_sesion(void) {
    int r;
    do {
        r = leer_entero("Rol (0=admin 1=operador 2=invitado): ");
        if (feof(stdin)) exit(0);
    } while (r < 0 || r >= NUM_ROLES);
    rol_actual = r;
}

const char *seg_usuario_actual(void) { return rol_actual < 0 ? "-" : NOMBRE_ROL[rol_actual]; }
int seg_obtener_rol(void) { return rol_actual; }

int seg_verificar(Objeto obj, unsigned char derecho) {
    if (rol_actual < 0) return 0;
    if ((matriz[rol_actual][obj] & derecho) == derecho) return 1;
    printf("\n[!] ACCESO DENEGADO (Fallo Seguro aplicado)\n");
    return 0;
}

void seg_guardar_matriz(void) {
    int fd = open(ARCH_MATRIZ, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd >= 0) { write(fd, matriz, sizeof matriz); fsync(fd); close(fd); }
}

void seg_cargar_matriz(void) {
    int fd = open(ARCH_MATRIZ, O_RDONLY);
    if (fd < 0) { seg_guardar_matriz(); return; }
    if (read(fd, matriz, sizeof matriz) != sizeof matriz)
        printf("matriz.dat dañada: se usan los valores por defecto\n");
    close(fd);
}

void seg_ver_matriz(void) {
    printf("\n--- MATRIZ DE ACCESO ---\n");
    for(int r = 0; r < NUM_ROLES; r++) {
        printf("%-10s | ", NOMBRE_ROL[r]);
        for(int o = 0; o < NUM_OBJETOS; o++) {
            printf("%c%c%c%c%c ", 
                (matriz[r][o] & D_CREATE) ? 'C' : '-',
                (matriz[r][o] & D_READ)   ? 'R' : '-',
                (matriz[r][o] & D_UPDATE) ? 'U' : '-',
                (matriz[r][o] & D_DELETE) ? 'D' : '-',
                (matriz[r][o] & D_EXEC)   ? 'X' : '-');
        }
        printf("\n");
    }
}

void seg_modificar_matriz(void) {
    int r = leer_entero("ID Rol (0=admin, 1=operador, 2=invitado): ");
    int o = leer_entero("ID Objeto (0=reg, 1=juegos, 2=part, 3=proc, 4=mat, 5=sist): ");
    int d = leer_entero("Valor de derechos (0-31): ");
    
    if (r >= 0 && r < NUM_ROLES && o >= 0 && o < NUM_OBJETOS) {
        if (r == ROL_ADMIN && o == OBJ_MATRIZ && !(d & D_UPDATE)) {
            printf("Error: El admin no puede quitarse permisos de matriz (Separación de privilegios).\n");
            return;
        }
        matriz[r][o] = d;
        seg_guardar_matriz();
        printf("Matriz actualizada exitosamente.\n");
    }
}
