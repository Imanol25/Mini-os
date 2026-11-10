#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <string.h>
#include <pthread.h>
#include "kernel.h"
#include "seguridad.h"

#define MAX_COLA 200
typedef struct { int x, y; } Punto;
typedef struct {
    char  jugador[32];
    int   puntaje, nivel, direccion, largo;
    Punto cuerpo[MAX_COLA]; Punto comida; long guardada_en;
} Partida;

static struct termios original;
static Partida partida_act; 
static pthread_mutex_t mutex_partida = PTHREAD_MUTEX_INITIALIZER;
static volatile int jugando = 0;

static void modo_juego(int activar) {
    if (activar) {
        struct termios t; tcgetattr(STDIN_FILENO, &original);
        t = original; t.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    } else tcsetattr(STDIN_FILENO, TCSANOW, &original);
}

static int tecla(int ms) {
    fd_set fds; struct timeval tv = { 0, ms * 1000 };
    FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        char c; if (read(STDIN_FILENO, &c, 1) == 1) return c;
    }
    return -1;
}

static int guardar_partida(const Partida *p) {
    char ruta[64]; snprintf(ruta, sizeof ruta, "datos/partida_%s.sav", p->jugador);
    int fd = open(ruta, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return -1;
    write(fd, p, sizeof *p); fsync(fd); close(fd);
    return 0;
}

static void *hilo_autoguardado(void *arg) {
    int segundos = *(int *)arg;
    while (jugando) {
        sleep(segundos);
        Partida copia;
        pthread_mutex_lock(&mutex_partida);
        copia = partida_act;
        pthread_mutex_unlock(&mutex_partida);
        guardar_partida(&copia);
    }
    return NULL;
}

static void bucle_juego(void) {
    modo_juego(1);
    int cada = 15; pthread_t hilo; jugando = 1;
    pthread_create(&hilo, NULL, hilo_autoguardado, &cada);

    while (jugando) {
        int t = tecla(150);
        if (t == 'q') { jugando = 0; break; }
        
        pthread_mutex_lock(&mutex_partida);
        if (t == 'g') { guardar_partida(&partida_act); jugando = 0; pthread_mutex_unlock(&mutex_partida); break; }
        partida_act.puntaje += 10; int pt = partida_act.puntaje;
        pthread_mutex_unlock(&mutex_partida);

        printf("\033[H\033[J"); 
        printf("--- MINI SNAKE ---\nPuntaje: %d | (q) Salir (g) Guardar\n", pt);
    }
    pthread_join(hilo, NULL); modo_juego(0);
}

void juego_nuevo(void) {
    pthread_mutex_lock(&mutex_partida);
    memset(&partida_act, 0, sizeof partida_act);
    strncpy(partida_act.jugador, seg_usuario_actual(), sizeof partida_act.jugador - 1);
    pthread_mutex_unlock(&mutex_partida);
    bucle_juego();
}

void juego_continuar(void) {
    Partida p; char ruta[64];
    snprintf(ruta, sizeof ruta, "datos/partida_%s.sav", seg_usuario_actual());
    int fd = open(ruta, O_RDONLY);
    if (fd < 0) { printf("No hay partida.\n"); return; }
    read(fd, &p, sizeof p); close(fd);
    
    pthread_mutex_lock(&mutex_partida);
    partida_act = p;
    pthread_mutex_unlock(&mutex_partida);
    bucle_juego();
}
