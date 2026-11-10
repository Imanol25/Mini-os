#ifndef MINIOS_H
#define MINIOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>

// --- SEGURIDAD ---
#define ROL_ADMIN 0
#define ROL_INVITADO 1
#define OBJ_SISTEMA 0
#define OBJ_PARTIDAS 1
#define OBJ_JUEGOS 2

#define MAX_ROLES 2
#define MAX_OBJ 3

#define D_READ 1
#define D_WRITE 2
#define D_EXEC 4
#define D_CREATE 8

extern int usuario_actual;
extern int matriz_rbac[MAX_ROLES][MAX_OBJ];

void seg_init();
int seg_verificar(int objeto, int permiso);

// --- SISTEMA DE ARCHIVOS (FS) ---
typedef struct {
    int id;
    int propietario;
    char descripcion[64];
    int puntaje;
} Registro_FS;

typedef struct {
    unsigned int magic_number;
    int sucio;
    unsigned char mapa_bits[32]; // 256 bloques teóricos
} Superbloque;

void fs_init();
void fs_estado();
void fs_listar();
void fs_actualizar(int id, const char *nueva_desc);
void fs_borrar(int id);
void fs_escribir(Registro_FS r);

// --- PROCESOS ---
typedef struct Proceso {
    int pid;
    int usuario;
    time_t inicio;
    int estado; // 0=Running, 1=Stopped
    struct Proceso *sig;
} Proceso;

void proc_init();
void proc_listar();
void proc_reanudar(int pid);
void proc_suspender(int pid);
void proc_crear(int pid);

// --- JUEGO ---
void bucle_juego();

#endif
