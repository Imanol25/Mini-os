# MiniOS 3.0 - Parte 1

## Integrantes
- [TU NOMBRE COMPLETO] - [TU CÓDIGO]

## Justificación Matriz RBAC

| Rol | Objeto | Permisos | Justificación |
| :--- | :--- | :--- | :--- |
| **Admin** | `OBJ_SISTEMA` | `D_READ \| D_EXEC` | El administrador debe leer "Acerca de" y ejecutar rutinas del sistema. |
| **Admin** | `OBJ_PARTIDAS`| `D_READ \| D_WRITE \| D_CREATE` | Lista registros guardados, escribe, actualiza y crea records. |
| **Admin** | `OBJ_JUEGOS` | `D_READ \| D_EXEC` | Requiere permiso `D_EXEC` para lanzar "Nueva Partida". |
| **Invitado** | `OBJ_SISTEMA` | `0` (Ninguno) | Sin acceso a info técnica. |
| **Invitado** | `OBJ_PARTIDAS`| `D_READ` | Solo puede leer los puntajes. No modifica ni borra. |
| **Invitado** | `OBJ_JUEGOS` | `D_READ \| D_EXEC` | Se autoriza ejecutar (`D_EXEC`) el juego de la serpiente. |
