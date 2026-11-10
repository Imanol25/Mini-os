#include <stdio.h>
#include "kernel.h"
#include "fs.h"
#include "seguridad.h"
#include "proc.h"
#include "juego.h"
#include "sistema.h"
#include "bitacora.h"
#include "virtualizacion.h"

static void mostrar_menu(void) {
    printf("\n===== MiniOS v%s (Parte 2) | usuario: %s =====\n", VERSION_MINIOS, seg_usuario_actual());
    printf(" --- ARCHIVOS (RAID 1) ---\n");
    printf(" 1. Create  2. Read  3. Update  4. Delete  5. Estado Disco\n");
    printf(" --- PROCESOS Y JUEGOS ---\n");
    printf(" 6. Lanzar Tetris  7. Listar Proc. (Memoria) 8. Reanudar Proc.  9. Terminar Proc.\n");
    printf(" 10. Juego: Nueva Partida  11. Juego: Continuar Partida\n");
    printf(" --- SEGURIDAD ---\n");
    printf(" 12. Ver matriz  13. Modificar matriz  14. Gestionar Usuarios  15. Cambiar Clave\n");
    printf(" 16. Ver bitacora\n");
    printf(" --- SISTEMA ---\n");
    printf(" 17. Info Virtualizacion  18. Acerca de  19. Cerrar sesion  0. Apagar\n");
}

int main(void) {
    if (fs_montar(DIR_DATOS "minios.disk") != 0) return 1;
    seg_cargar_matriz();
    auditar("ARRANQUE", "sistema", "OK");

    while (1) {
        while (!seg_login()) { } 
        int en_sesion = 1;
        while (en_sesion) {
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
                case 14:if (seg_verificar(OBJ_USUARIOS, D_UPDATE))  seg_gestionar_usuarios(); break;
                case 15:if (seg_verificar(OBJ_USUARIOS, D_UPDATE))  seg_cambiar_clave(); break;
                case 16:if (seg_verificar(OBJ_BITACORA, D_READ))    bitacora_mostrar(); break;
                case 17:if (seg_verificar(OBJ_VIRTUALIZACION, D_EXEC)) virt_info(); break;
                case 18:if (seg_verificar(OBJ_SISTEMA, D_READ))     acerca_de();     break;
                case 19: seg_cerrar_sesion(); en_sesion = 0; break;
                case 0:
                    if (seg_verificar(OBJ_SISTEMA, D_EXEC)) {
                        auditar("APAGADO", "sistema", "OK"); fs_desmontar(); return 0;
                    }
                    break;
                default:
                    if (feof(stdin)) { fs_desmontar(); return 0; }
                    printf("Opción inválida.\n");
            }
        }
    }
}
