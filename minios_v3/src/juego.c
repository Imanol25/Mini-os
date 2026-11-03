#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <string.h>
#include <time.h>
#include "kernel.h"
#include "seguridad.h"

#define MAX_COLA 200
typedef struct { int x, y; } Punto;
typedef struct {
    char  jugador[32];
    int   puntaje, nivel, direccion, largo;
    Punto cuerpo[MAX_COLA];
    Punto comida;
    long  guardada_en;
} Partida;

static struct termios original;

static void modo_juego(int activar) {
    if (activar) {
        struct termios t;
        tcgetattr(STDIN_FILENO, &original);
        t = original;
        t.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
    }
}

static int tecla(int ms) {
    fd_set fds;
    struct timeval tv = { 0, ms * 1000 };
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) return c;
    }
    return -1;
}

static int guardar_partida(const Partida *p) {
    char ruta[64];
    snprintf(ruta, sizeof ruta, "datos/partida_%s.sav", p->jugador);
    int fd = open(ruta, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return -1;
    write(fd, p, sizeof *p);
    fsync(fd);
    close(fd);
    return 0;
}

static void bucle_juego(Partida *p) {
    modo_juego(1);
    while (1) {
        int t = tecla(150);
        if (t == 'g') { guardar_partida(p); break; }
        if (t == 'q') break;
        p->puntaje += 10; 
        printf("\033[H\033[J"); 
        printf("--- MINI SNAKE (Simulacion) ---\n");
        printf("Puntaje: %d | (q) Salir (g) Guardar estado\n", p->puntaje);
    }
    modo_juego(0);
}

void juego_nuevo(void) {
    Partida p;
    memset(&p, 0, sizeof p);
    strncpy(p.jugador, seg_usuario_actual(), sizeof p.jugador - 1);
    bucle_juego(&p);
}

void juego_continuar(void) {
    Partida p;
    char ruta[64];
    snprintf(ruta, sizeof ruta, "datos/partida_%s.sav", seg_usuario_actual());
    int fd = open(ruta, O_RDONLY);
    if (fd < 0) { printf("No tienes ninguna partida guardada.\n"); return; }
    read(fd, &p, sizeof p);
    close(fd);
    bucle_juego(&p);
}
