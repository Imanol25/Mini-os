```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ARCHIVO "registros.dat"
#define TEMPORAL "temporal.dat"

#define MAX_NOMBRE 50
#define MAX_DESCRIPCION 150

typedef struct {
    int id;
    char nombre[MAX_NOMBRE];
    int edad;
    char descripcion[MAX_DESCRIPCION];
} Registro;


/* =====================================================
   FUNCIONES AUXILIARES
   ===================================================== */

void limpiarBuffer(void) {
    int c;

    while ((c = getchar()) != '\n' && c != EOF) {
    }
}


void pausar(void) {
    printf("\nPresione ENTER para continuar...");
    getchar();
}


void limpiarPantalla(void) {
    printf("\033[2J\033[H");
}


void leerCadena(const char *mensaje, char *cadena, int tamano) {

    do {
        printf("%s", mensaje);

        if (fgets(cadena, tamano, stdin) == NULL) {
            cadena[0] = '\0';
            continue;
        }

        cadena[strcspn(cadena, "\n")] = '\0';

        if (strlen(cadena) == 0) {
            printf("El campo no puede estar vacio.\n");
        }

    } while (strlen(cadena) == 0);
}


int leerEntero(const char *mensaje) {

    char entrada[100];
    char *fin;
    long numero;

    while (1) {

        printf("%s", mensaje);

        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            continue;
        }

        numero = strtol(entrada, &fin, 10);

        while (isspace((unsigned char)*fin)) {
            fin++;
        }

        if (*fin == '\0') {

            if (numero >= 0 && numero <= 2147483647) {
                return (int)numero;
            }
        }

        printf("Entrada invalida. Ingrese un numero valido.\n");
    }
}


/* =====================================================
   BUSCAR ID
   ===================================================== */

int existeID(int id) {

    FILE *archivo;
    Registro registro;

    archivo = fopen(ARCHIVO, "rb");

    if (archivo == NULL) {
        return 0;
    }

    while (fread(&registro, sizeof(Registro), 1, archivo) == 1) {

        if (registro.id == id) {
            fclose(archivo);
            return 1;
        }
    }

    fclose(archivo);

    return 0;
}


/* =====================================================
   CREATE
   ===================================================== */

void crearRegistro(void) {

    FILE *archivo;
    Registro nuevo;

    limpiarPantalla();

    printf("============================================\n");
    printf("              CREAR REGISTRO\n");
    printf("============================================\n\n");

    nuevo.id = leerEntero("ID: ");

    if (existeID(nuevo.id)) {

        printf("\nERROR: Ya existe un registro con ese ID.\n");

        pausar();
        return;
    }

    leerCadena(
        "Nombre: ",
        nuevo.nombre,
        MAX_NOMBRE
    );

    do {

        nuevo.edad = leerEntero("Edad: ");

        if (nuevo.edad > 120) {
            printf("La edad debe estar entre 0 y 120.\n");
        }

    } while (nuevo.edad > 120);

    leerCadena(
        "Descripcion: ",
        nuevo.descripcion,
        MAX_DESCRIPCION
    );

    archivo = fopen(ARCHIVO, "ab");

    if (archivo == NULL) {

        printf("\nERROR: No se pudo abrir el archivo.\n");

        pausar();
        return;
    }

    if (fwrite(&nuevo, sizeof(Registro), 1, archivo) != 1) {

        printf("\nERROR: No se pudo guardar el registro.\n");

        fclose(archivo);
        pausar();
        return;
    }

    fclose(archivo);

    printf("\n============================================\n");
    printf("Registro creado correctamente.\n");
    printf("ID: %d\n", nuevo.id);
    printf("============================================\n");

    pausar();
}


/* =====================================================
   READ
   ===================================================== */

void leerRegistros(void) {

    FILE *archivo;
    Registro registro;

    int cantidad = 0;

    limpiarPantalla();

    printf("============================================\n");
    printf("             REGISTROS ALMACENADOS\n");
    printf("============================================\n");

    archivo = fopen(ARCHIVO, "rb");

    if (archivo == NULL) {

        printf("\nNo existen registros almacenados.\n");

        pausar();
        return;
    }

    while (fread(&registro, sizeof(Registro), 1, archivo) == 1) {

        cantidad++;

        printf("\n--------------------------------------------\n");
        printf("Registro #%d\n", cantidad);
        printf("--------------------------------------------\n");

        printf("ID          : %d\n", registro.id);
        printf("Nombre      : %s\n", registro.nombre);
        printf("Edad        : %d\n", registro.edad);
        printf("Descripcion : %s\n", registro.descripcion);
    }

    fclose(archivo);

    if (cantidad == 0) {
        printf("\nEl archivo no contiene registros.\n");
    }

    printf("\nTotal de registros: %d\n", cantidad);

    pausar();
}


/* =====================================================
   READ INDIVIDUAL
   ===================================================== */

void buscarRegistro(void) {

    FILE *archivo;
    Registro registro;

    int id;
    int encontrado = 0;

    limpiarPantalla();

    printf("============================================\n");
    printf("              BUSCAR REGISTRO\n");
    printf("============================================\n\n");

    id = leerEntero("Ingrese el ID: ");

    archivo = fopen(ARCHIVO, "rb");

    if (archivo == NULL) {

        printf("\nNo existen registros.\n");

        pausar();
        return;
    }

    while (fread(&registro, sizeof(Registro), 1, archivo) == 1) {

        if (registro.id == id) {

            printf("\nRegistro encontrado:\n");
            printf("--------------------------------------------\n");
            printf("ID          : %d\n", registro.id);
            printf("Nombre      : %s\n", registro.nombre);
            printf("Edad        : %d\n", registro.edad);
            printf("Descripcion : %s\n", registro.descripcion);

            encontrado = 1;
            break;
        }
    }

    fclose(archivo);

    if (!encontrado) {
        printf("\nNo existe un registro con el ID %d.\n", id);
    }

    pausar();
}


/* =====================================================
   UPDATE
   ===================================================== */

void actualizarRegistro(void) {

    FILE *archivo;
    Registro registro;

    int id;
    int encontrado = 0;

    limpiarPantalla();

    printf("============================================\n");
    printf("             ACTUALIZAR REGISTRO\n");
    printf("============================================\n\n");

    id = leerEntero("Ingrese el ID que desea actualizar: ");

    archivo = fopen(ARCHIVO, "rb+");

    if (archivo == NULL) {

        printf("\nNo existen registros.\n");

        pausar();
        return;
    }

    while (fread(&registro, sizeof(Registro), 1, archivo) == 1) {

        if (registro.id == id) {

            encontrado = 1;

            printf("\nRegistro encontrado.\n\n");

            printf("Nombre actual: %s\n", registro.nombre);

            leerCadena(
                "Nuevo nombre: ",
                registro.nombre,
                MAX_NOMBRE
            );

            do {

                registro.edad = leerEntero("Nueva edad: ");

                if (registro.edad > 120) {
                    printf("La edad debe estar entre 0 y 120.\n");
                }

            } while (registro.edad > 120);

            leerCadena(
                "Nueva descripcion: ",
                registro.descripcion,
                MAX_DESCRIPCION
            );

            fseek(
                archivo,
                -(long)sizeof(Registro),
                SEEK_CUR
            );

            if (fwrite(
                &registro,
                sizeof(Registro),
                1,
                archivo
            ) != 1) {

                printf("\nERROR al actualizar el registro.\n");

            } else {

                printf("\nRegistro actualizado correctamente.\n");
            }

            break;
        }
    }

    fclose(archivo);

    if (!encontrado) {

        printf(
            "\nNo existe un registro con el ID %d.\n",
            id
        );
    }

    pausar();
}


/* =====================================================
   DELETE
   ===================================================== */

void eliminarRegistro(void) {

    FILE *archivo;
    FILE *temporal;

    Registro registro;

    int id;
    int encontrado = 0;

    limpiarPantalla();

    printf("============================================\n");
    printf("              ELIMINAR REGISTRO\n");
    printf("============================================\n\n");

    id = leerEntero("Ingrese el ID que desea eliminar: ");

    archivo = fopen(ARCHIVO, "rb");

    if (archivo == NULL) {

        printf("\nNo existen registros.\n");

        pausar();
        return;
    }

    temporal = fopen(TEMPORAL, "wb");

    if (temporal == NULL) {

        printf("\nERROR: No se pudo crear archivo temporal.\n");

        fclose(archivo);

        pausar();
        return;
    }

    while (fread(&registro, sizeof(Registro), 1, archivo) == 1) {

        if (registro.id == id) {

            encontrado = 1;

            continue;
        }

        fwrite(
            &registro,
            sizeof(Registro),
            1,
            temporal
        );
    }

    fclose(archivo);
    fclose(temporal);

    if (!encontrado) {

        remove(TEMPORAL);

        printf(
            "\nNo existe un registro con el ID %d.\n",
            id
        );

        pausar();
        return;
    }

    if (remove(ARCHIVO) != 0) {

        printf("\nERROR al eliminar el archivo original.\n");

        remove(TEMPORAL);

        pausar();
        return;
    }

    if (rename(TEMPORAL, ARCHIVO) != 0) {

        printf("\nERROR al reemplazar el archivo.\n");

        pausar();
        return;
    }

    printf("\nRegistro eliminado correctamente.\n");

    pausar();
}


/* =====================================================
   CONTAR REGISTROS
   ===================================================== */

int contarRegistros(void) {

    FILE *archivo;
    Registro registro;

    int cantidad = 0;

    archivo = fopen(ARCHIVO, "rb");

    if (archivo == NULL) {
        return 0;
    }

    while (fread(&registro, sizeof(Registro), 1, archivo) == 1) {
        cantidad++;
    }

    fclose(archivo);

    return cantidad;
}


/* =====================================================
   INSTALAR / EJECUTAR BSDGAMES
   ===================================================== */

void iniciarBSDGames(void) {

    int opcion;

    limpiarPantalla();

    printf("============================================\n");
    printf("                 BSDGAMES\n");
    printf("============================================\n\n");

    printf("1. Tetris\n");
    printf("2. Snake\n");
    printf("3. Salir\n\n");

    opcion = leerEntero("Seleccione el juego: ");

    switch (opcion) {

        case 1:

            printf("\nIniciando Tetris...\n");

            if (system("command -v tetris > /dev/null 2>&1") == 0) {

                system("tetris");

            } else {

                printf("\nTetris no esta instalado.\n");
                printf("Puedes instalarlo con:\n\n");
                printf("sudo apt install bsdgames\n");
            }

            break;


        case 2:

            printf("\nIniciando Snake...\n");

            if (system("command -v snake > /dev/null 2>&1") == 0) {

                system("snake");

            } else {

                printf("\nSnake no esta instalado.\n");
                printf("Puedes instalar BSDGames con:\n\n");
                printf("sudo apt install bsdgames\n");
            }

            break;


        case 3:

            printf("\nRegresando al menu principal...\n");

            break;


        default:

            printf("\nOpcion invalida.\n");
    }

    pausar();
}


/* =====================================================
   ESTADO DEL SISTEMA
   ===================================================== */

void mostrarEstado(void) {

    int cantidad;

    limpiarPantalla();

    cantidad = contarRegistros();

    printf("============================================\n");
    printf("              ESTADO DEL SISTEMA\n");
    printf("============================================\n\n");

    printf("Sistema       : MINI OS\n");
    printf("Almacenamiento: %s\n", ARCHIVO);
    printf("Registros     : %d\n", cantidad);

    if (cantidad > 0) {
        printf("Estado CRUD   : ACTIVO\n");
    } else {
        printf("Estado CRUD   : SIN REGISTROS\n");
    }

    if (system("command -v tetris > /dev/null 2>&1") == 0) {
        printf("Tetris        : DISPONIBLE\n");
    } else {
        printf("Tetris        : NO INSTALADO\n");
    }

    printf("\n============================================\n");

    pausar();
}


/* =====================================================
   MENU
   ===================================================== */

void mostrarMenu(void) {

    printf("\n");
    printf("============================================\n");
    printf("            MINI SISTEMA OPERATIVO\n");
    printf("============================================\n");

    printf("\nGESTION DE MEMORIA SECUNDARIA\n");
    printf("--------------------------------------------\n");

    printf("1. CREATE  - Crear registro\n");
    printf("2. READ    - Leer registros\n");
    printf("3. UPDATE  - Actualizar registro\n");
    printf("4. DELETE  - Eliminar registro\n");
    printf("5. SEARCH  - Buscar registro\n");

    printf("\nGESTION DE PROCESOS\n");
    printf("--------------------------------------------\n");

    printf("6. BSDGames - Ejecutar juego\n");

    printf("\nCONTROL DEL SISTEMA\n");
    printf("--------------------------------------------\n");

    printf("7. Estado del sistema\n");
    printf("8. Apagar sistema\n");

    printf("\n============================================\n");
}


/* =====================================================
   MAIN
   ===================================================== */

int main(void) {

    int opcion;

    limpiarPantalla();

    printf("============================================\n");
    printf("       INICIANDO MINI SISTEMA OPERATIVO\n");
    printf("============================================\n");

    printf("\nSistema iniciado correctamente.\n");

    while (1) {

        mostrarMenu();

        opcion = leerEntero("Seleccione una opcion: ");

        switch (opcion) {

            case 1:
                crearRegistro();
                break;

            case 2:
                leerRegistros();
                break;

            case 3:
                actualizarRegistro();
                break;

            case 4:
                eliminarRegistro();
                break;

            case 5:
                buscarRegistro();
                break;

            case 6:
                iniciarBSDGames();
                break;

            case 7:
                mostrarEstado();
                break;

            case 8:

                limpiarPantalla();

                printf("============================================\n");
                printf("          APAGANDO MINI SISTEMA\n");
                printf("============================================\n");

                printf("\nCerrando sistema...\n");
                printf("Guardando estado...\n");
                printf("Procesos finalizados.\n");
                printf("\nMINI OS apagado correctamente.\n\n");

                return 0;

            default:

                printf(
                    "\nOpcion invalida. "
                    "Seleccione entre 1 y 8.\n"
                );

                break;
        }
    }

    return 0;
}
```
