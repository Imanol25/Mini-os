#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include "kernel.h"
#include "seguridad.h"
#include "bitacora.h"

#define ARCH_MATRIZ DIR_DATOS "matriz.dat"
#define ARCH_USUARIOS DIR_DATOS "usuarios.dat"
#define MAX_INTENTOS 3

typedef struct {
    char          nombre[32];
    unsigned long sal, hash;
    int           rol, intentos_fallidos, bloqueado;
} Usuario;

static Usuario sesion;
static int hay_sesion = 0;

static unsigned char matriz[NUM_ROLES][NUM_OBJETOS] = {
/* admin    */ { 0x0F, 0x10, 0x02, 0x0E, 0x06, 0x10, 0x07, 0x02, 0x10 },
/* operador */ { 0x07, 0x10, 0x07, 0x0E, 0x02, 0x00, 0x00, 0x00, 0x10 },
/* invitado */ { 0x02, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

static const char *NOMBRE_ROL[NUM_ROLES] = { "admin", "operador", "invitado" };
static const char *NOMBRE_OBJ[NUM_OBJETOS] = { "registros", "juegos", "partidas", "procesos", "matriz", "sistema", "usuarios", "bitacora", "virtualiz" };

static unsigned long hash_clave(const char *clave, unsigned long sal) {
    unsigned long h = 5381 ^ sal;
    for (const unsigned char *p = (const unsigned char *)clave; *p; p++) h = ((h << 5) + h) + *p;
    return h;
}

static void leer_clave(const char *prompt, char *buf, int n) {
    struct termios viejo, nuevo; int tty = isatty(STDIN_FILENO);
    if (tty) {
        tcgetattr(STDIN_FILENO, &viejo); nuevo = viejo; nuevo.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &nuevo);
    }
    leer_linea(prompt, buf, n);
    if (tty) { tcsetattr(STDIN_FILENO, TCSANOW, &viejo); printf("\n"); }
}

static void crear_usuario(const char *nombre, const char *clave, Rol rol) {
    Usuario u; memset(&u, 0, sizeof u);
    strncpy(u.nombre, nombre, sizeof u.nombre - 1);
    u.sal = ((unsigned long)rand() << 16) ^ (unsigned long)time(NULL);
    u.hash = hash_clave(clave, u.sal); u.rol = rol;
    int fd = open(ARCH_USUARIOS, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd >= 0) { write(fd, &u, sizeof u); close(fd); }
}

static int buscar_usuario(const char *nombre, Usuario *u, off_t *pos) {
    int fd = open(ARCH_USUARIOS, O_RDONLY); if (fd < 0) return 0;
    off_t p = 0;
    while (read(fd, u, sizeof *u) == sizeof *u) {
        if (strcmp(u->nombre, nombre) == 0) { *pos = p; close(fd); return 1; }
        p += sizeof *u;
    }
    close(fd); return 0;
}

static void guardar_usuario(const Usuario *u, off_t pos) {
    int fd = open(ARCH_USUARIOS, O_WRONLY);
    if (fd >= 0) { lseek(fd, pos, SEEK_SET); write(fd, u, sizeof *u); fsync(fd); close(fd); }
}

int seg_login(void) {
    if (access(ARCH_USUARIOS, F_OK) != 0) {
        srand((unsigned)time(NULL) ^ (unsigned)getpid());
        crear_usuario("admin", "admin123", ROL_ADMIN);
        crear_usuario("operador", "oper123", ROL_OPERADOR);
        crear_usuario("invitado", "inv123", ROL_INVITADO);
    }
    char nombre[32], clave[64]; Usuario u; off_t pos;
    printf("\n--- LOGIN MINIOS ---\n");
    leer_linea("Usuario: ", nombre, sizeof nombre); if (feof(stdin)) exit(0);
    leer_clave("Contraseña: ", clave, sizeof clave);

    if (!buscar_usuario(nombre, &u, &pos)) {
        auditar("LOGIN", nombre, "LOGIN FALLIDO");
        printf("Usuario o contraseña incorrectos.\n"); return 0;
    }
    if (u.bloqueado) {
        auditar("LOGIN", nombre, "CUENTA BLOQUEADA");
        printf("Cuenta bloqueada. Contacte al administrador.\n"); return 0;
    }
    if (hash_clave(clave, u.sal) != u.hash) {
        u.intentos_fallidos++;
        if (u.intentos_fallidos >= MAX_INTENTOS) u.bloqueado = 1;
        guardar_usuario(&u, pos);
        auditar("LOGIN", nombre, u.bloqueado ? "BLOQUEO" : "LOGIN FALLIDO");
        printf("Usuario o contraseña incorrectos.\n"); return 0;
    }
    u.intentos_fallidos = 0; guardar_usuario(&u, pos);
    sesion = u; hay_sesion = 1;
    auditar("LOGIN", nombre, "LOGIN EXITOSO"); return 1;
}

void seg_cerrar_sesion(void) {
    auditar("LOGOUT", sesion.nombre, "OK");
    hay_sesion = 0; memset(&sesion, 0, sizeof sesion);
}
const char *seg_usuario_actual(void) { return hay_sesion ? sesion.nombre : "-"; }
const char *seg_rol_actual(void) { return hay_sesion ? NOMBRE_ROL[sesion.rol] : "-"; }
int seg_obtener_rol(void) { return hay_sesion ? sesion.rol : -1; }

int seg_verificar(Objeto obj, unsigned char derecho) {
    char accion[16]; snprintf(accion, sizeof accion, "DERECHO_%02X", derecho);
    if (hay_sesion && (matriz[sesion.rol][obj] & derecho) == derecho) {
        auditar(accion, NOMBRE_OBJ[obj], "PERMITIDO"); return 1;
    }
    auditar(accion, NOMBRE_OBJ[obj], "DENEGADO");
    printf("  ACCESO DENEGADO\n"); return 0;
}

void seg_guardar_matriz(void) {
    int fd = open(ARCH_MATRIZ, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd >= 0) { write(fd, matriz, sizeof matriz); fsync(fd); close(fd); }
}

void seg_cargar_matriz(void) {
    int fd = open(ARCH_MATRIZ, O_RDONLY);
    if (fd < 0) { seg_guardar_matriz(); return; }
    if (read(fd, matriz, sizeof matriz) != sizeof matriz) printf("Matriz dañada.\n");
    close(fd);
}

void seg_ver_matriz(void) {
    printf("\n--- MATRIZ DE ACCESO ---\n");
    for(int r = 0; r < NUM_ROLES; r++) {
        printf("%-10s | ", NOMBRE_ROL[r]);
        for(int o = 0; o < NUM_OBJETOS; o++) {
            printf("%c%c%c%c%c ", (matriz[r][o] & D_CREATE) ? 'C' : '-', (matriz[r][o] & D_READ) ? 'R' : '-',
                   (matriz[r][o] & D_UPDATE) ? 'U' : '-', (matriz[r][o] & D_DELETE) ? 'D' : '-', (matriz[r][o] & D_EXEC) ? 'X' : '-');
        }
        printf("\n");
    }
}

void seg_modificar_matriz(void) {
    int r = leer_entero("ID Rol (0=admin, 1=operador, 2=invitado): ");
    int o = leer_entero("ID Objeto (0 a 8): ");
    int d = leer_entero("Valor derechos (0-31): ");
    if (r >= 0 && r < NUM_ROLES && o >= 0 && o < NUM_OBJETOS) {
        if (r == ROL_ADMIN && o == OBJ_MATRIZ && !(d & D_UPDATE)) {
            printf("Error: Admin no puede quitarse permisos de matriz.\n"); return;
        }
        matriz[r][o] = d; seg_guardar_matriz();
        printf("Matriz actualizada.\n");
    }
}

void seg_gestionar_usuarios(void) {
    int op = leer_entero("\n1. Listar  2. Crear  3. Desbloquear | Opcion: ");
    if (op == 1) {
        int fd = open(ARCH_USUARIOS, O_RDONLY); if (fd < 0) return;
        Usuario u; printf("\n--- USUARIOS ---\n");
        while (read(fd, &u, sizeof u) == sizeof u)
            printf("Usuario: %-10s | Rol: %-10s | Estado: %s\n", u.nombre, NOMBRE_ROL[u.rol], u.bloqueado ? "BLOQUEADO" : "ACTIVO");
        close(fd);
    } else if (op == 2) {
        char n[32], c[64]; leer_linea("Nuevo usuario: ", n, sizeof n); leer_clave("Clave: ", c, sizeof c);
        int rol = leer_entero("Rol (0=admin 1=oper 2=inv): ");
        if (rol >= 0 && rol <= 2) { crear_usuario(n, c, rol); printf("Usuario creado.\n"); }
    } else if (op == 3) {
        char n[32]; leer_linea("Usuario a desbloquear: ", n, sizeof n);
        Usuario u; off_t pos;
        if (buscar_usuario(n, &u, &pos)) {
            u.bloqueado = 0; u.intentos_fallidos = 0; guardar_usuario(&u, pos);
            printf("Usuario desbloqueado.\n");
        } else printf("No encontrado.\n");
    }
}

void seg_cambiar_clave(void) {
    char actual[64], nueva[64]; leer_clave("Clave actual: ", actual, sizeof actual);
    Usuario u; off_t pos;
    if (buscar_usuario(sesion.nombre, &u, &pos) && hash_clave(actual, u.sal) == u.hash) {
        leer_clave("Nueva clave (min 6 chars): ", nueva, sizeof nueva);
        if (strlen(nueva) >= 6) {
            u.sal = ((unsigned long)rand() << 16) ^ (unsigned long)time(NULL);
            u.hash = hash_clave(nueva, u.sal); guardar_usuario(&u, pos);
            printf("Clave cambiada exitosamente.\n");
        } else printf("Error: La clave debe tener mínimo 6 caracteres.\n");
    } else printf("Clave actual incorrecta.\n");
}
