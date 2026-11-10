#include "../include/minios.h"

#define DISK_FILE "minios.disk"

Superbloque sb;
int fd_disco = -1;

void fs_init() {
    fd_disco = open(DISK_FILE, O_RDWR | O_CREAT, 0666);
    if (fd_disco < 0) {
        printf("Error al montar el disco virtual.\n");
        return;
    }
    
    if (read(fd_disco, &sb, sizeof(Superbloque)) != sizeof(Superbloque)) {
        sb.magic_number = 0xEF53;
        sb.sucio = 0;
        memset(sb.mapa_bits, 0, sizeof(sb.mapa_bits));
        lseek(fd_disco, 0, SEEK_SET);
        write(fd_disco, &sb, sizeof(Superbloque));
        fsync(fd_disco);
    }
}

void fs_estado() {
    printf("\n--- Estado del Sistema de Archivos ---\n");
    printf("Magic Number : 0x%X\n", sb.magic_number);
    printf("Estado disco : %s\n", sb.sucio ? "Sucio (1)" : "Limpio (0)");
    printf("Mapa de bits : %02X %02X %02X %02X\n", 
           sb.mapa_bits[0], sb.mapa_bits[1], sb.mapa_bits[2], sb.mapa_bits[3]);
}

void fs_listar() {
    lseek(fd_disco, sizeof(Superbloque), SEEK_SET);
    Registro_FS r;
    int count = 0;
    
    printf("\n--- Registros en minios.disk ---\n");
    while (read(fd_disco, &r, sizeof(Registro_FS)) == sizeof(Registro_FS)) {
        if (r.id != 0) { 
            printf("ID: %d | Propietario: %s | Puntaje: %d | Desc: %s\n", 
                   r.id, r.propietario == ROL_ADMIN ? "Admin" : "Invitado", r.puntaje, r.descripcion);
            count++;
        }
    }
    if (count == 0) printf("No hay partidas guardadas en el disco.\n");
}

void fs_actualizar(int id, const char *nueva_desc) {
    lseek(fd_disco, sizeof(Superbloque), SEEK_SET);
    Registro_FS r;
    off_t pos = sizeof(Superbloque);
    
    while (read(fd_disco, &r, sizeof(Registro_FS)) == sizeof(Registro_FS)) {
        if (r.id == id) {
            if (r.propietario != usuario_actual && usuario_actual != ROL_ADMIN) {
                printf("Error: No eres el propietario del registro %d.\n", id);
                return;
            }
            
            strncpy(r.descripcion, nueva_desc, sizeof(r.descripcion) - 1);
            r.descripcion[sizeof(r.descripcion) - 1] = '\0';
            
            lseek(fd_disco, pos, SEEK_SET);
            write(fd_disco, &r, sizeof(Registro_FS));
            sb.sucio = 1;
            
            fsync(fd_disco);
            lseek(fd_disco, 0, SEEK_SET);
            write(fd_disco, &sb, sizeof(Superbloque));
            fsync(fd_disco);
            
            printf("Registro %d actualizado.\n", id);
            return;
        }
        pos += sizeof(Registro_FS);
    }
    printf("Registro no encontrado.\n");
}

void fs_borrar(int id) {
    lseek(fd_disco, sizeof(Superbloque), SEEK_SET);
    Registro_FS r;
    off_t pos = sizeof(Superbloque);
    
    while (read(fd_disco, &r, sizeof(Registro_FS)) == sizeof(Registro_FS)) {
        if (r.id == id) {
            if (r.propietario != usuario_actual && usuario_actual != ROL_ADMIN) {
                printf("Error: No eres el propietario.\n");
                return;
            }
            
            r.id = 0; 
            lseek(fd_disco, pos, SEEK_SET);
            write(fd_disco, &r, sizeof(Registro_FS));
            
            sb.sucio = 1;
            lseek(fd_disco, 0, SEEK_SET);
            write(fd_disco, &sb, sizeof(Superbloque));
            fsync(fd_disco);
            
            printf("Registro %d borrado.\n", id);
            return;
        }
        pos += sizeof(Registro_FS);
    }
    printf("Registro no encontrado.\n");
}

void fs_escribir(Registro_FS nueva_r) {
    lseek(fd_disco, 0, SEEK_END);
    write(fd_disco, &nueva_r, sizeof(Registro_FS));
    sb.sucio = 1; 
    lseek(fd_disco, 0, SEEK_SET);
    write(fd_disco, &sb, sizeof(Superbloque));
    fsync(fd_disco);
}
