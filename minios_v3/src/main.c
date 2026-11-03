#include <stdio.h>
#include "kernel.h"
#include "fs.h"
#include "seguridad.h"
#include "proc.h"
#include "juego.h"
#include "sistema.h"

static void mostrar_menu(void) {
    printf("\n===== MiniOS v%s | usuario: %s =====\n", VERSION_MINIOS, seg_usuario_actual());
    printf(" --- ARCHIVOS (Disco Virtual) ---\n");
    printf(" 1. Create  2. Read  3. Update  4. Delete  5. Estado Disco\n");
    printf(" --- PROCESOS Y JUEGOS ---\n");
    printf(" 6. Lanzar Tetris  7. Listar Proc.  8. Reanudar Proc.  9. Terminar Proc.\n");
    printf(" 10. Juego: Nueva Partida  11. Juego: Continuar Partida\n");
    printf(" --- SEGURIDAD ---\n");
    printf(" 12. Ver matriz  13. Modificar matriz\n");
    printf(" --- SISTEMA ---\n");
    printf(" 14. Acerca de  0. Apagar\n");
}

int main(void) {
    if (fs_montar(DIR_DATOS "minios.disk") != 0) return 1;
    seg_cargar_matriz();
    seg_iniciar_sesion();

    while (1) {
        mostrar_menu();
        switch (leer_entero(" Opción: ")) {
            case 1: if (seg_verificar(OBJ_REGISTROS, D_CREATE)) fs_crear();      break;
            case 2: if (seg_verificar(OBJ_REGISTROS, D_READ))   fs_listar();     break;
            case 3: if (seg_verificar(OBJ_REGISTROS, D_UPDATE)) fs_actualizar(); break;
            case 4: if (seg_verificar(OBJ_REGISTROS, D_DELETE)) fs_borrar();     break;
            case 5: if (seg_verificar(OBJ_REGISTROS, D_READ))   fs_estado();     break;
            case 6: if (seg_verificar(OBJ_JUEGOS, D_EXEC))      proc_lanzar("tetris-bsd"); break;
            case 7: if (seg_verificar(OBJ_PROCESOS, D_READ))    proc_listar();   break;
            case 8: if (seg_verificar(OBJ_PROCESOS, D_UPDATE))  proc_reanudar(); break;
            case 9: if (seg_verificar(OBJ_PROCESOS, D_DELETE))  proc_terminar(); break;
            case 10:if (seg_verificar(OBJ_PARTIDAS, D_CREATE))  juego_nuevo();   break;
            case 11:if (seg_verificar(OBJ_PARTIDAS, D_READ))    juego_continuar(); break;
            case 12:if (seg_verificar(OBJ_MATRIZ, D_READ))      seg_ver_matriz(); break;
            case 13:if (seg_verificar(OBJ_MATRIZ, D_UPDATE))    seg_modificar_matriz(); break;
            case 14:if (seg_verificar(OBJ_SISTEMA, D_READ))     acerca_de();     break;
            case 0:
                if (seg_verificar(OBJ_SISTEMA, D_EXEC)) {
                    fs_desmontar();
                    printf("Apagando MiniOS...\n");
                    return 0;
                }
                break;
            default:
                if (feof(stdin)) { fs_desmontar(); return 0; }
                printf("Opción inválida.\n");
        }
    }
}
