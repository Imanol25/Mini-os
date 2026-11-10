#include "../include/minios.h"
#include <sys/select.h>

int kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

void bucle_juego() {
    printf("\n--- Iniciando MINI SNAKE ---\n");
    printf("Controles: [W] Arriba, [S] Abajo, [A] Izquierda, [D] Derecha. [Q] Salir.\n");
    sleep(2);
    
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);          
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int ancho = 30, alto = 15;
    int x = ancho / 2, y = alto / 2;
    int comida_x = rand() % ancho, comida_y = rand() % alto;
    int puntaje = 0;
    char dir = 'd';
    int jugando = 1;

    while(jugando) {
        if (kbhit()) {
            char tecla = getchar();
            if (tecla == 'w' || tecla == 's' || tecla == 'a' || tecla == 'd') dir = tecla;
            if (tecla == 'q') jugando = 0;
        }

        if (dir == 'w') y--;
        if (dir == 's') y++;
        if (dir == 'a') x--;
        if (dir == 'd') x++;

        if (x < 0 || x >= ancho || y < 0 || y >= alto) {
            printf("\n\n¡CHOQUE! Has perdido.\n");
            break;
        }

        if (x == comida_x && y == comida_y) {
            puntaje += 10;
            comida_x = rand() % ancho;
            comida_y = rand() % alto;
        }

        printf("\r[Snake] Pos:(%2d,%2d) | Comida:(%2d,%2d) | PUNTAJE: %3d  ", x, y, comida_x, comida_y, puntaje);
        fflush(stdout);
        usleep(150000); 
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\nPartida Finalizada. Puntaje Final: %d\n", puntaje);

    Registro_FS r;
    r.id = (int)time(NULL) % 10000; 
    r.propietario = usuario_actual;
    r.puntaje = puntaje;
    snprintf(r.descripcion, sizeof(r.descripcion), "Partida Snake. Pts: %d", puntaje);
    
    fs_escribir(r);
    printf("-> Record guardado exitosamente en el disco virtual [ID: %d].\n", r.id);
}
