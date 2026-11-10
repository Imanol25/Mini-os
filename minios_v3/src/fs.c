#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "fs.h"
#include "seguridad.h"
#include "kernel.h"

#define MAGIC 0x4D494E49 
#define MAX_REG 100

typedef struct {
    unsigned int magic, version, total, usados, siguiente_id, limpio;
} Superbloque;

typedef struct {
    int  id, activo; 
    char nombre[50], descripcion[100], propietario[32];
} Registro;

static int fd_disco = -1, fd_espejo = -1;
static Superbloque sb;

static off_t pos_registro(int i) { return (off_t)sizeof(Superbloque) + (off_t)i * sizeof(Registro); }

static int leer_registro(int i, Registro *r) {
    lseek(fd_disco, pos_registro(i), SEEK_SET);
    return read(fd_disco, r, sizeof *r) == sizeof *r ? 0 : -1;
}

static int escribir_en_ambos(off_t pos, const void *datos, size_t n) {
    int ok = 0;
    if (lseek(fd_disco,  pos, SEEK_SET) >= 0 && write(fd_disco,  datos, n) == (ssize_t)n) ok++;
    if (lseek(fd_espejo, pos, SEEK_SET) >= 0 && write(fd_espejo, datos, n) == (ssize_t)n) ok++;
    return ok == 2 ? 0 : -1;
}

static void guardar_superbloque(void) {
    escribir_en_ambos(0, &sb, sizeof sb);
    fsync(fd_disco); fsync(fd_espejo);
}

static int copiar_disco(int origen, int destino) {
    char bloque[4096]; ssize_t n;
    lseek(origen, 0, SEEK_SET); lseek(destino, 0, SEEK_SET);
    while ((n = read(origen, bloque, sizeof bloque)) > 0)
        if (write(destino, bloque, n) != n) return -1;
    fsync(destino); return 0;
}

int fs_montar(const char *ruta) {
    char ruta_espejo[256]; snprintf(ruta_espejo, sizeof ruta_espejo, "%s_espejo.disk", ruta);
    fd_disco = open(ruta, O_RDWR | O_CREAT, 0600);
    fd_espejo = open(ruta_espejo, O_RDWR | O_CREAT, 0600);
    if (fd_disco < 0 || fd_espejo < 0) return -1;

    Superbloque sb1, sb2;
    int r1 = read(fd_disco, &sb1, sizeof sb1);
    int r2 = read(fd_espejo, &sb2, sizeof sb2);
    int d1_ok = (r1 == sizeof sb1 && sb1.magic == MAGIC);
    int d2_ok = (r2 == sizeof sb2 && sb2.magic == MAGIC);

    if (d1_ok && !d2_ok) {
        printf("Disco espejo dañado/faltante. Reconstruyendo desde el principal...\n");
        copiar_disco(fd_disco, fd_espejo); sb = sb1;
    } else if (!d1_ok && d2_ok) {
        printf("Disco principal dañado/faltante. Reconstruyendo desde el espejo...\n");
        copiar_disco(fd_espejo, fd_disco); sb = sb2;
    } else if (!d1_ok && !d2_ok) {
        printf("Ambos discos sin formato: formateando RAID 1...\n");
        memset(&sb, 0, sizeof sb);
        sb.magic = MAGIC; sb.version = 3; sb.total = MAX_REG;
        sb.siguiente_id = 1; sb.limpio = 1;
        guardar_superbloque();
        Registro vacio; memset(&vacio, 0, sizeof vacio);
        for (int i = 0; i < MAX_REG; i++) escribir_en_ambos(pos_registro(i), &vacio, sizeof vacio);
    } else sb = sb1;

    if (!sb.limpio) printf("ADVERTENCIA: el disco no se desmontó correctamente.\n");
    sb.limpio = 0; guardar_superbloque(); return 0;
}

void fs_desmontar(void) {
    sb.limpio = 1; guardar_superbloque();
    close(fd_disco); close(fd_espejo); fd_disco = -1; fd_espejo = -1;
}

void fs_crear(void) {
    Registro r;
    for(int i=0; i<MAX_REG; i++) {
        leer_registro(i, &r);
        if(r.activo == 0) {
            r.id = sb.siguiente_id++; r.activo = 1;
            leer_linea("Nombre: ", r.nombre, sizeof(r.nombre));
            leer_linea("Descripcion: ", r.descripcion, sizeof(r.descripcion));
            strncpy(r.propietario, seg_usuario_actual(), sizeof(r.propietario)-1);
            escribir_en_ambos(pos_registro(i), &r, sizeof r);
            sb.usados++; guardar_superbloque();
            printf("Registro %d creado en ambos discos.\n", r.id); return;
        }
    }
}

void fs_listar(void) {
    Registro r; printf("\n--- REGISTROS ---\n");
    for(int i=0; i<MAX_REG; i++) {
        leer_registro(i, &r);
        if(r.activo) printf("ID: %d | Nombre: %s | Propietario: %s\n", r.id, r.nombre, r.propietario);
    }
}

void fs_actualizar(void) {
    int id = leer_entero("ID a actualizar: "); Registro r;
    for(int i=0; i<MAX_REG; i++) {
        leer_registro(i, &r);
        if(r.activo && r.id == id) {
            if(seg_obtener_rol() == ROL_OPERADOR && strcmp(r.propietario, seg_usuario_actual()) != 0) {
                printf("Propiedad denegada.\n"); return;
            }
            leer_linea("Nuevo nombre: ", r.nombre, sizeof(r.nombre));
            escribir_en_ambos(pos_registro(i), &r, sizeof r);
            printf("Registro actualizado en espejo.\n"); return;
        }
    }
}

void fs_borrar(void) {
    int id = leer_entero("ID a borrar: "); Registro r;
    for(int i=0; i<MAX_REG; i++) {
        leer_registro(i, &r);
        if(r.activo && r.id == id) {
            r.activo = 0; escribir_en_ambos(pos_registro(i), &r, sizeof r);
            sb.usados--; guardar_superbloque();
            printf("Borrado lógico exitoso.\n"); return;
        }
    }
}

void fs_estado(void) {
    printf("Superbloque -> Usados: %d, Libres: %d, Sig ID: %d (RAID 1)\n", sb.usados, sb.total - sb.usados, sb.siguiente_id);
}
