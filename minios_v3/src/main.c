#include "../include/minios.h"

void mostrar_acerca_de() {
    if (!seg_verificar(OBJ_SISTEMA, D_READ)) {
        printf("Error: Acceso denegado. No tienes permisos para leer sistema.\n");
        return;
    }
    printf("\n--- Acerca del Sistema ---\n");
    printf("MiniOS Version 3.0 (Parte 1 Completada)\n");
}

void iniciar_juego() {
    if (!seg_verificar(OBJ_JUEGOS, D_EXEC)) {
        printf("Error: Acceso denegado. No puedes ejecutar juegos.\n");
        return;
    }
    bucle_juego();
}

void cambiar_usuario() {
    printf("\nSeleccione el nuevo rol (0 = Admin, 1 = Invitado): ");
    int nuevo_rol;
    if (scanf("%d", &nuevo_rol) == 1) {
        if (nuevo_rol == ROL_ADMIN || nuevo_rol == ROL_INVITADO) {
            usuario_actual = nuevo_rol;
            printf("Cambio exitoso. Ahora eres: %s\n", usuario_actual == ROL_ADMIN ? "Admin" : "Invitado");
        } else {
            printf("Rol invalido.\n");
        }
    }
}

int main() {
    seg_init();
    fs_init();
    proc_init();
    proc_crear(getpid()); 

    int opcion;
    do {
        printf("\n==================================\n");
        printf("   MINIOS 3.0 | Rol actual: %s\n", usuario_actual == ROL_ADMIN ? "Admin" : "Invitado");
        printf("==================================\n");
        printf("1. FS: Ver estado del disco\n");
        printf("2. FS: Listar todos los registros\n");
        printf("3. FS: Actualizar descripcion\n");
        printf("4. FS: Borrar un registro\n");
        printf("5. PROC: Listar procesos\n");
        printf("6. PROC: Suspender proceso\n");
        printf("7. PROC: Reanudar proceso\n");
        printf("10. JUEGO: Nueva Partida Snake\n");
        printf("14. Acerca del sistema\n");
        printf("15. SISTEMA: Cambiar de usuario/rol\n"); 
        printf("0. Salir\n");
        printf("Seleccione una opcion: ");
        
        if (scanf("%d", &opcion) != 1) break;
        
        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        switch(opcion) {
            case 1: fs_estado(); break;
            case 2: fs_listar(); break;
            case 3: {
                int id; char desc[64];
                printf("ID a modificar: "); scanf("%d", &id);
                while ((c = getchar()) != '\n' && c != EOF); 
                printf("Nueva descripcion: ");
                fgets(desc, sizeof(desc), stdin);
                desc[strcspn(desc, "\n")] = 0; 
                fs_actualizar(id, desc);
                break;
            }
            case 4: {
                int id;
                printf("ID a borrar: "); scanf("%d", &id);
                fs_borrar(id);
                break;
            }
            case 5: proc_listar(); break;
            case 6: {
                int pid;
                printf("PID a suspender: "); scanf("%d", &pid);
                proc_suspender(pid);
                break;
            }
            case 7: {
                int pid;
                printf("PID a reanudar: "); scanf("%d", &pid);
                proc_reanudar(pid);
                break;
            }
            case 10: iniciar_juego(); break;
            case 14: mostrar_acerca_de(); break;
            case 15: cambiar_usuario(); break;
            case 0: printf("\nSaliendo...\n"); break;
            default: printf("Opcion invalida.\n");
        }
    } while (opcion != 0);

    return 0;
}
