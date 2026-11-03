#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "fs.h"
#include "seguridad.h"
#include "kernel.h"

#define MAGIC   0x4D494E49 
#define MAX_REG 100

typedef struct {
    unsigned int magic, version, total, usados, siguiente_id, limpio;
} Superbloque;

typedef struct {
    int  id, activo; 
    char nombre[50], descripcion[100], propietario[32];
} Registro;

static int disco_fd = -1;
static Superbloque sb;

static off_t pos_registro(int i) { return (off_t)sizeof(Superbloque) + (off_t)i * sizeof(Registro); }

static int leer_registro(int i, Registro *r) {
    lseek(disco_fd, pos_registro(i), SEEK_SET);
    return read(disco_fd, r, sizeof *r) == sizeof *r ? 0 : -1;
}

static int escribir_registro(int i, const Registro *r) {
    lseek(disco_fd, pos_registro(i), SEEK_SET);
    return write(disco_fd, r, sizeof *r) == sizeof *r ? 0 : -1;
}

static void guardar_superbloque(void) {
    lseek(disco_fd, 0, SEEK_SET);
    write(disco_fd, &sb, sizeof sb);
    fsync(disco_fd);
}

int fs_montar(const char *ruta) {
    disco_fd = open(ruta, O_RDWR | O_CREAT, 0600);
    if (disco_fd < 0) { perror("open"); return -1; }
    if (read(disco_fd, &sb, sizeof sb) != sizeof sb || sb.magic != MAGIC) {
        printf("Disco sin formato: formateando...\n");
        memset(&sb, 0, sizeof sb);
        sb.magic = MAGIC; sb.version = 3; sb.total = MAX_REG;
        sb.siguiente_id = 1; sb.limpio = 1;
        Registro vacio;
        memset(&vacio, 0, sizeof vacio);
        for (int i = 0; i < MAX_REG; i++) escribir_registro(i, &vacio);
    }
    if (!sb.limpio) printf("ADVERTENCIA: el disco no se desmontó correctamente (Sucio).\n");
    sb.limpio = 0;
    guardar_superbloque();
    return 0;
}

void fs_desmontar(void) {
    sb.limpio = 1;
    guardar_superbloque();
    close(disco_fd);
    disco_fd = -1;
}

void fs_crear(void) {
    Registro r;
    for(int i=0; i<MAX_REG; i++) {
        leer_registro(i, &r);
        if(r.activo == 0) {
            r.id = sb.siguiente_id++;
            r.activo = 1;
            leer_linea("Nombre: ", r.nombre, sizeof(r.nombre));
            leer_linea("Descripcion: ", r.descripcion, sizeof(r.descripcion));
            strncpy(r.propietario, seg_usuario_actual(), sizeof(r.propietario)-1);
            escribir_registro(i, &r);
            sb.usados++;
            guardar_superbloque();
            printf("Registro %d creado en disco.\n", r.id);
            return;
        }
    }
    printf("Disco lleno.\n");
}

void fs_listar(void) {
    Registro r;
    printf("\n--- REGISTROS ---\n");
    for(int i=0; i<MAX_REG; i++) {
        leer_registro(i, &r);
        if(r.activo) printf("ID: %d | Nombre: %s | Propietario: %s\n", r.id, r.nombre, r.propietario);
    }
}

void fs_actualizar(void) {
    int id = leer_entero("ID a actualizar: ");
    Registro r;
    for(int i=0; i<MAX_REG; i++) {
        leer_registro(i, &r);
        if(r.activo && r.id == id) {
            if(seg_obtener_rol() == ROL_OPERADOR && strcmp(r.propietario, seg_usuario_actual()) != 0) {
                printf("Propiedad denegada: Solo puedes modificar tus registros.\n"); return;
            }
            leer_linea("Nuevo nombre: ", r.nombre, sizeof(r.nombre));
            escribir_registro(i, &r);
            printf("Registro actualizado.\n");
            return;
        }
    }
}

void fs_borrar(void) {
    int id = leer_entero("ID a borrar: ");
    Registro r;
    for(int i=0; i<MAX_REG; i++) {
        leer_registro(i, &r);
        if(r.activo && r.id == id) {
            r.activo = 0; 
            escribir_registro(i, &r);
            sb.usados--;
            guardar_superbloque();
            printf("Borrado lógico exitoso (espacio liberado sin corromper el disco).\n");
            return;
        }
    }
}

void fs_estado(void) {
    printf("Superbloque -> Usados: %d, Libres: %d, Siguiente ID: %d\n", sb.usados, sb.total - sb.usados, sb.siguiente_id);
}
