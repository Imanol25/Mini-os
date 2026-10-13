#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <ctype.h>
#include <pwd.h>

#define PATH_MAX 4096
#define BUFFER_SIZE 4096

char BASE_DIR[PATH_MAX];
char LOG_FILE[PATH_MAX];

/* =====================================================
   FUNCIONES AUXILIARES Y DE SEGURIDAD
   ===================================================== */
void pausar(void) {
    printf("\nPresione ENTER para continuar...");
    while(getchar() != '\n');
}

void limpiarPantalla(void) {
    printf("\033[2J\033[H");
}

void registrar_bitacora(const char *accion) {
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        time_t t = time(NULL);
        char *fecha = ctime(&t);
        fecha[strlen(fecha)-1] = '\0';
        char buffer[1024];
        int len = snprintf(buffer, sizeof(buffer), "[%s] %s\n", fecha, accion);
        write(fd, buffer, len);
        close(fd);
    }
}

// Inicializa el directorio base ~/miniSO/disk/
void inicializar_entorno() {
    const char *homedir = getenv("HOME");
    if (homedir == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    snprintf(BASE_DIR, sizeof(BASE_DIR), "%s/miniSO/disk", homedir);
    snprintf(LOG_FILE, sizeof(LOG_FILE), "%s/miniSO/bitacora.log", homedir);

    char temp_dir[PATH_MAX];
    snprintf(temp_dir, sizeof(temp_dir), "%s/miniSO", homedir);
    mkdir(temp_dir, 0755); 
    mkdir(BASE_DIR, 0755); 
    registrar_bitacora("Sistema iniciado");
}

// Verifica que la ruta no salga de ~/miniSO/disk/ (Aislamiento)
int es_ruta_segura(const char *ruta_relativa, char *ruta_absoluta) {
    char temp[PATH_MAX + 512]; // Tamaño aumentado para evitar -Wformat-truncation
    snprintf(temp, sizeof(temp), "%s/%s", BASE_DIR, ruta_relativa);
    
    char *res = realpath(temp, ruta_absoluta);
    if (res == NULL) {
        // Si el archivo no existe (ej. para crear uno nuevo), validamos el directorio padre
        strncpy(ruta_absoluta, temp, PATH_MAX);
        return strstr(ruta_absoluta, BASE_DIR) == ruta_absoluta;
    }
    return strncmp(ruta_absoluta, BASE_DIR, strlen(BASE_DIR)) == 0;
}

int leerEntero(const char *mensaje) {
    char entrada[100];
    printf("%s", mensaje);
    if (fgets(entrada, sizeof(entrada), stdin) == NULL) return -1;
    return atoi(entrada);
}

void leerCadena(const char *mensaje, char *cadena, int tamano) {
    printf("%s", mensaje);
    if (fgets(cadena, tamano, stdin) != NULL) {
        cadena[strcspn(cadena, "\n")] = '\0';
    }
}

/* =====================================================
   GESTOR DE ARCHIVOS (MEMORIA SECUNDARIA)
   ===================================================== */
void listar_archivos() {
    DIR *dir = opendir(BASE_DIR);
    if (!dir) {
        perror("Error abriendo directorio");
        return;
    }
    struct dirent *entrada;
    struct stat st;
    char ruta[PATH_MAX + 512]; // Tamaño aumentado para evitar -Wformat-truncation
    
    printf("\n--- CONTENIDO DE %s ---\n", BASE_DIR);
    printf("%-20s %-10s %-10s %-10s %s\n", "Nombre", "Tipo", "Size(B)", "Permisos", "Modificacion");
    printf("----------------------------------------------------------------------\n");
    
    while ((entrada = readdir(dir)) != NULL) {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) continue;
        
        snprintf(ruta, sizeof(ruta), "%s/%s", BASE_DIR, entrada->d_name);
        if (stat(ruta, &st) == 0) {
            char mod_time[64];
            strftime(mod_time, sizeof(mod_time), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));
            
            printf("%-20s %-10s %-10ld %-10o %s\n", 
                   entrada->d_name, 
                   S_ISDIR(st.st_mode) ? "Dir" : "Archivo",
                   (long)st.st_size, 
                   st.st_mode & 0777, 
                   mod_time);
        }
    }
    closedir(dir);
    registrar_bitacora("Listó archivos");
}

void crear_archivo_texto() {
    char nombre[256], contenido[BUFFER_SIZE], ruta[PATH_MAX];
    leerCadena("Nombre del archivo: ", nombre, sizeof(nombre));
    
    if (!es_ruta_segura(nombre, ruta)) {
        printf("Error: Violación de seguridad (ruta inválida).\n");
        return;
    }

    int fd = open(ruta, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error al crear archivo");
        return;
    }

    leerCadena("Ingrese el contenido: ", contenido, sizeof(contenido));
    if (write(fd, contenido, strlen(contenido)) == -1) perror("Error escribiendo");
    else printf("Archivo '%s' creado.\n", nombre);
    
    close(fd);
    registrar_bitacora("Creó archivo de texto");
}

void crear_directorio() {
    char nombre[256], ruta[PATH_MAX];
    leerCadena("Nombre del directorio: ", nombre, sizeof(nombre));
    if (!es_ruta_segura(nombre, ruta)) return;

    if (mkdir(ruta, 0755) == -1) perror("Error creando directorio");
    else printf("Directorio creado exitosamente.\n");
    registrar_bitacora("Creó directorio");
}

void leer_archivo() {
    char nombre[256], ruta[PATH_MAX];
    leerCadena("Nombre del archivo a leer: ", nombre, sizeof(nombre));
    if (!es_ruta_segura(nombre, ruta)) return;

    struct stat st;
    if (stat(ruta, &st) == -1) {
        perror("Error accediendo al archivo");
        return;
    }

    int fd = open(ruta, O_RDONLY);
    if (fd == -1) {
        perror("Error abriendo archivo");
        return;
    }

    // Heurística simple para determinar si es texto
    char buf[64];
    int n = read(fd, buf, 64);
    int es_texto = 1;
    for(int i=0; i<n; i++) {
        if (!isprint(buf[i]) && buf[i] != '\n' && buf[i] != '\r' && buf[i] != '\t') {
            es_texto = 0; break;
        }
    }

    lseek(fd, 0, SEEK_SET); // Rebobinar

    printf("\n--- LEYENDO ARCHIVO ---\n");
    if (es_texto) {
        char buffer[BUFFER_SIZE];
        ssize_t bytes;
        while ((bytes = read(fd, buffer, sizeof(buffer)-1)) > 0) {
            buffer[bytes] = '\0';
            printf("%s", buffer);
        }
    } else {
        printf("Metadatos - Tamaño: %ld bytes\n", (long)st.st_size);
        printf("Primeros 64 bytes (Hex):\n");
        char hexbuf[64];
        ssize_t bytes = read(fd, hexbuf, 64);
        for (int i = 0; i < bytes; i++) {
            printf("%02x ", (unsigned char)hexbuf[i]);
            if ((i+1) % 16 == 0) printf("\n");
        }
        printf("\n");
    }
    close(fd);
    registrar_bitacora("Leyó archivo");
}

void copiar_archivo() {
    char origen[256], destino[256], ruta_origen[PATH_MAX], ruta_destino[PATH_MAX];
    leerCadena("Archivo origen: ", origen, sizeof(origen));
    if (!es_ruta_segura(origen, ruta_origen)) return;
    leerCadena("Archivo destino: ", destino, sizeof(destino));
    if (!es_ruta_segura(destino, ruta_destino)) return;

    int fd_in = open(ruta_origen, O_RDONLY);
    if (fd_in == -1) { perror("Error origen"); return; }
    
    int fd_out = open(ruta_destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out == -1) { perror("Error destino"); close(fd_in); return; }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_leidos, bytes_escritos;
    while ((bytes_leidos = read(fd_in, buffer, BUFFER_SIZE)) > 0) {
        bytes_escritos = write(fd_out, buffer, bytes_leidos);
        if (bytes_escritos != bytes_leidos) {
            perror("Error de escritura"); break;
        }
    }
    
    struct stat st_in, st_out;
    fstat(fd_in, &st_in); fstat(fd_out, &st_out);
    
    close(fd_in); close(fd_out);

    if (st_in.st_size == st_out.st_size) {
        printf("Copia exitosa. Integridad verificada (%ld bytes).\n", (long)st_in.st_size);
    } else {
        printf("ADVERTENCIA: La copia difiere en tamaño.\n");
    }
    registrar_bitacora("Copió archivo");
}

void mover_renombrar() {
    char origen[256], destino[256], ruta_origen[PATH_MAX], ruta_destino[PATH_MAX];
    leerCadena("Ruta/Nombre actual: ", origen, sizeof(origen));
    if (!es_ruta_segura(origen, ruta_origen)) return;
    leerCadena("Nuevo Nombre/Ruta: ", destino, sizeof(destino));
    if (!es_ruta_segura(destino, ruta_destino)) return;

    if (rename(ruta_origen, ruta_destino) == -1) perror("Error al renombrar/mover");
    else printf("Operación exitosa.\n");
    registrar_bitacora("Movió/Renombró archivo");
}

void eliminar_archivo_directorio() {
    char nombre[256], ruta[PATH_MAX];
    leerCadena("Archivo/Directorio a eliminar: ", nombre, sizeof(nombre));
    if (!es_ruta_segura(nombre, ruta)) return;

    struct stat st;
    if (stat(ruta, &st) == -1) { perror("Error stat"); return; }

    char conf[10];
    leerCadena("¿Seguro que desea eliminar? (s/n): ", conf, sizeof(conf));
    if (conf[0] != 's' && conf[0] != 'S') {
        printf("Operación cancelada.\n"); return;
    }

    if (S_ISDIR(st.st_mode)) {
        if (rmdir(ruta) == -1) perror("Error al eliminar directorio (¿está vacío?)");
        else printf("Directorio eliminado.\n");
    } else {
        if (unlink(ruta) == -1) perror("Error al eliminar archivo");
        else printf("Archivo eliminado.\n");
    }
    registrar_bitacora("Eliminó archivo/directorio");
}

void menu_gestor_archivos() {
    int opcion;
    do {
        limpiarPantalla();
        printf("=== GESTOR DE ARCHIVOS ===\n");
        printf("Disco: %s\n\n", BASE_DIR);
        printf("1. Listar contenido\n");
        printf("2. Crear archivo de texto\n");
        printf("3. Crear directorio\n");
        printf("4. Leer archivo\n");
        printf("5. Copiar archivo (con integridad)\n");
        printf("6. Renombrar o Mover\n");
        printf("7. Eliminar archivo/directorio\n");
        printf("11. Volver al menu principal\n");
        
        opcion = leerEntero("\nOpcion: ");
        switch (opcion) {
            case 1: listar_archivos(); pausar(); break;
            case 2: crear_archivo_texto(); pausar(); break;
            case 3: crear_directorio(); pausar(); break;
            case 4: leer_archivo(); pausar(); break;
            case 5: copiar_archivo(); pausar(); break;
            case 6: mover_renombrar(); pausar(); break;
            case 7: eliminar_archivo_directorio(); pausar(); break;
            case 11: break;
            default: printf("Opcion invalida.\n"); pausar(); break;
        }
    } while (opcion != 11);
}

/* =====================================================
   CENTRO DE JUEGOS (GESTION DE PROCESOS)
   ===================================================== */
void ejecutar_juego(const char *comando) {
    char ruta[PATH_MAX];
    snprintf(ruta, sizeof(ruta), "/usr/games/%s", comando); 

    if (access(ruta, X_OK) == -1) {
        if (access("/usr/bin/tetris-bsd", X_OK) != -1 && strcmp(comando, "tetris-bsd")==0) {
            snprintf(ruta, sizeof(ruta), "/usr/bin/tetris-bsd");
        } else if (access("/usr/bin/snake", X_OK) != -1 && strcmp(comando, "snake")==0) {
            snprintf(ruta, sizeof(ruta), "/usr/bin/snake");
        } else {
            perror("Error: El juego no existe o no es ejecutable");
            printf("Ruta intentada: %s\n", ruta);
            return;
        }
    }

    struct termios terminal_original;
    tcgetattr(STDIN_FILENO, &terminal_original);

    time_t inicio = time(NULL);
    
    pid_t pid = fork(); 
    if (pid == -1) {
        perror("Error en fork()");
        return;
    } else if (pid == 0) {
        // PROCESO HIJO
        signal(SIGINT, SIG_DFL); 
        char *args[] = {(char *)comando, NULL};
        execv(ruta, args);
        perror("Fallo execv");
        _exit(127); 
    } else {
        // PROCESO PADRE
        signal(SIGINT, SIG_IGN); 
        int status;
        waitpid(pid, &status, 0); 
        
        // Restaurar estado
        tcsetattr(STDIN_FILENO, TCSANOW, &terminal_original);
        signal(SIGINT, SIG_DFL);
        
        time_t fin = time(NULL);
        double duracion = difftime(fin, inicio);

        printf("\n--- INFORME DE PARTIDA ---\n");
        printf("PID del hijo: %d\n", pid);
        if (WIFEXITED(status)) {
            printf("Terminacion: Normal (Codigo %d)\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Terminacion: Por Señal (Señal %d)\n", WTERMSIG(status));
        }
        printf("Duracion: %.0f segundos\n", duracion);
        
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Jugó %s durante %.0f s", comando, duracion);
        registrar_bitacora(log_msg);
    }
}

void menu_centro_juegos() {
    int opcion;
    do {
        limpiarPantalla();
        printf("=== CENTRO DE JUEGOS ===\n");
        printf("1. Tetris (tetris-bsd)\n");
        printf("2. Snake\n");
        printf("3. Volver al menu principal\n");
        
        opcion = leerEntero("\nOpcion: ");
        switch (opcion) {
            case 1: ejecutar_juego("tetris-bsd"); pausar(); break;
            case 2: ejecutar_juego("snake"); pausar(); break;
            case 3: break;
            default: printf("Opcion invalida.\n"); pausar(); break;
        }
    } while (opcion != 3);
}

/* =====================================================
   BITACORA DEL SISTEMA
   ===================================================== */
void ver_bitacora() {
    limpiarPantalla();
    printf("=== BITACORA DEL SISTEMA ===\n\n");
    int fd = open(LOG_FILE, O_RDONLY);
    if (fd == -1) {
        printf("La bitacora esta vacia o no existe.\n");
    } else {
        char buffer[BUFFER_SIZE];
        ssize_t bytes;
        while ((bytes = read(fd, buffer, sizeof(buffer)-1)) > 0) {
            buffer[bytes] = '\0';
            printf("%s", buffer);
        }
        close(fd);
    }
    pausar();
}

/* =====================================================
   MAIN - BUCLE PRINCIPAL
   ===================================================== */
int main(void) {
    inicializar_entorno();
    int opcion;

    while (1) {
        limpiarPantalla();
        printf("========= MINI OS v2.0 =========\n");
        printf("1. Gestor de Archivos\n");
        printf("2. Centro de Juegos\n");
        printf("3. Ver Bitacora del Sistema\n");
        printf("0. Apagar Sistema\n");
        printf("================================\n");

        opcion = leerEntero("\nSeleccione una instruccion: ");

        switch (opcion) {
            case 1: menu_gestor_archivos(); break;
            case 2: menu_centro_juegos(); break;
            case 3: ver_bitacora(); break;
            case 0:
                limpiarPantalla();
                printf("Apagando el sistema de forma segura...\nProcesos terminados.\n");
                registrar_bitacora("Sistema apagado");
                return 0; 
            default:
                printf("\nComando no reconocido.\n");
                pausar();
                break;
        }
    }
    return 0;
}
