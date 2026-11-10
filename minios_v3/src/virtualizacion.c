#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "virtualizacion.h"
#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

static void leer_dmi(const char *campo, char *buf, int n) {
    char ruta[96]; snprintf(ruta, sizeof ruta, "/sys/class/dmi/id/%s", campo);
    FILE *f = fopen(ruta, "r"); strcpy(buf, "(no disponible)");
    if (f) { if (fgets(buf, n, f)) buf[strcspn(buf, "\n")] = '\0'; fclose(f); }
}

void virt_info(void) {
    printf("\n--- INFORMACION DE VIRTUALIZACION ---\n");
#if defined(__x86_64__) || defined(__i386__)
    unsigned int a, b, c, d; char fabricante[13] = {0};
    __get_cpuid(0, &a, &b, &c, &d);
    memcpy(fabricante, &b, 4); memcpy(fabricante + 4, &d, 4); memcpy(fabricante + 8, &c, 4);
    printf("CPU: %s\n", fabricante);
    __get_cpuid(1, &a, &b, &c, &d);
    int en_vm = (c >> 31) & 1;
    printf("Dentro de una VM (Bit Hypervisor): %s\n", en_vm ? "SÍ" : "NO");
    printf("VT-x expuesto (Intel): %s\n", ((c >> 5) & 1) ? "SÍ" : "NO");
    
    __cpuid(0x80000001, a, b, c, d);
    printf("AMD-V expuesto (AMD):  %s\n", ((c >> 2) & 1) ? "SÍ" : "NO");

    if (en_vm) {
        char firma[13] = {0};
        __cpuid(0x40000000, a, b, c, d);
        memcpy(firma, &b, 4); memcpy(firma + 4, &c, 4); memcpy(firma + 8, &d, 4);
        printf("Firma del hipervisor: %s\n", firma);
    }
#else
    printf("CPUID solo existe en x86.\n");
#endif
    char fab[96], prod[96];
    leer_dmi("sys_vendor", fab, sizeof fab); leer_dmi("product_name", prod, sizeof prod);
    printf("DMI: %s / %s\n", fab, prod);
    
    printf("systemd-detect-virt: "); fflush(stdout);
    if (system("systemd-detect-virt 2>/dev/null || echo no disponible") != 0) {}

    printf("Paginación anidada (EPT/NPT): "); fflush(stdout);
    if (system("grep -E 'ept|npt' /proc/cpuinfo > /dev/null && echo SÍ || echo NO") != 0) {}
    printf("\nConclusión: MiniOS corre dentro de un entorno virtualizado.\n");
}
