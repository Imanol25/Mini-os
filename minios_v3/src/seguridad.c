#include "../include/minios.h"

int usuario_actual = ROL_ADMIN; // Por defecto inicia como Admin
int matriz_rbac[MAX_ROLES][MAX_OBJ];

void seg_init() {
    matriz_rbac[ROL_ADMIN][OBJ_SISTEMA] = D_READ | D_EXEC; 
    matriz_rbac[ROL_ADMIN][OBJ_PARTIDAS] = D_READ | D_WRITE | D_CREATE;
    matriz_rbac[ROL_ADMIN][OBJ_JUEGOS] = D_READ | D_EXEC; 

    matriz_rbac[ROL_INVITADO][OBJ_SISTEMA] = 0; 
    matriz_rbac[ROL_INVITADO][OBJ_PARTIDAS] = D_READ;
    matriz_rbac[ROL_INVITADO][OBJ_JUEGOS] = D_READ | D_EXEC; 
}

int seg_verificar(int objeto, int permiso) {
    return (matriz_rbac[usuario_actual][objeto] & permiso) == permiso;
}
