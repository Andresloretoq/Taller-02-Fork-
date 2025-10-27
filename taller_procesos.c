/*****Integrantes: Andres Loreto Quiros y Santiago Hernandez *****/
/*Taller 2*/
/*Sistemas Operativos*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

static int *leer_enteros(const char *ruta, size_t n, size_t *n_leidos) {
    FILE *f = fopen(ruta, "r");
    if (!f) {
        fprintf(stderr, "Error abriendo '%s': %s\n", ruta, strerror(errno));
        return NULL;
    }
    int *v = (int *)malloc(n * sizeof(int));
    if (!v) {
        fprintf(stderr, "Error: sin memoria para %zu enteros\n", n);
        fclose(f);
        return NULL;
    }
    size_t i = 0;
    while (i < n && fscanf(f, "%d", &v[i]) == 1) {
        i++;
    }
    fclose(f);
    if (i < n) {
        fprintf(stderr, "Advertencia: se esperaban %zu enteros en '%s' pero se leyeron %zu.\n",
                n, ruta, i);
    }
    *n_leidos = i;
    return v;
}

static long suma_arreglo(const int *v, size_t n) {
    long s = 0;
    for (size_t i = 0; i < n; ++i) s += v[i];
    return s;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s N1 archivo00 N2 archivo01\n", argv[0]);
        return 1;
    }

    char *end = NULL;
    long N1_l = strtol(argv[1], &end, 10);
    if (*argv[1] == '\0' || *end != '\0' || N1_l < 0) {
        fprintf(stderr, "N1 inválido: %s\n", argv[1]);
        return 1;
    }
    const char *f0 = argv[2];
    long N2_l = strtol(argv[3], &end, 10);
    if (*argv[3] == '\0' || *end != '\0' || N2_l < 0) {
        fprintf(stderr, "N2 inválido: %s\n", argv[3]);
        return 1;
    }
    const char *f1 = argv[4];

    size_t n0_read = 0, n1_read = 0;
    int *a0 = leer_enteros(f0, (size_t)N1_l, &n0_read);
    if (!a0) return 1;
    int *a1 = leer_enteros(f1, (size_t)N2_l, &n1_read);
    if (!a1) { free(a0); return 1; }

 
    int pA[2], pB[2], pT[2];
    if (pipe(pA) == -1 || pipe(pB) == -1 || pipe(pT) == -1) {
        perror("pipe");
        free(a0); free(a1);
        return 1;
    }

    // ---- PRIMER HIJO ---- 
    pid_t pid_h1 = fork();
    if (pid_h1 < 0) {
        perror("fork hijo1");
        free(a0); free(a1);
        return 1;
    }
    if (pid_h1 == 0) {
        close(pA[0]); 
        close(pB[0]); close(pB[1]);
        close(pT[0]);

        // ---- NIETO ----
        pid_t pid_nieto = fork();
        if (pid_nieto < 0) {
            perror("fork nieto");

        }
        if (pid_nieto == 0) {
            //nieto: calcula sumaA de arreglo0
            long sumaA = suma_arreglo(a0, n0_read);
       
            if (write(pA[1], &sumaA, sizeof(sumaA)) < 0) {
                perror("write sumaA");
            }
            
            close(pA[1]);
            free(a0); free(a1);
            _exit(0);
        }

        // primer hijo: calcula la suma total
        long total = suma_arreglo(a0, n0_read) + suma_arreglo(a1, n1_read);
        if (write(pT[1], &total, sizeof(total)) < 0) {
            perror("write total");
        }
        close(pT[1]);      
        close(pA[1]);     
        waitpid(pid_nieto, NULL, 0);
        free(a0); free(a1);
        _exit(0);
    }

    // ---- SEGUNDO HIJO ----
    pid_t pid_h2 = fork();
    if (pid_h2 < 0) {
        perror("fork hijo2");
        
    }
    if (pid_h2 == 0) {
        // segundo hijo
        close(pA[0]); close(pA[1]);
        close(pB[0]); // 
        close(pT[0]); close(pT[1]);
        long sumaB = suma_arreglo(a1, n1_read);
        if (write(pB[1], &sumaB, sizeof(sumaB)) < 0) {
            perror("write sumaB");
        }
        close(pB[1]);
        free(a0); free(a1);
        _exit(0);
    }

    // ---- PADRE ----
    close(pA[1]);
    close(pB[1]);
    close(pT[1]);

    long sumaA_rec = 0, sumaB_rec = 0, total_rec = 0;
    ssize_t r;

    r = read(pA[0], &sumaA_rec, sizeof(sumaA_rec));
    if (r <= 0) fprintf(stderr, "Padre: no pudo leer sumaA (r=%zd)\n", r);
    r = read(pB[0], &sumaB_rec, sizeof(sumaB_rec));
    if (r <= 0) fprintf(stderr, "Padre: no pudo leer sumaB (r=%zd)\n", r);
    r = read(pT[0], &total_rec, sizeof(total_rec));
    if (r <= 0) fprintf(stderr, "Padre: no pudo leer total (r=%zd)\n", r);

    close(pA[0]); close(pB[0]); close(pT[0]);

    //espera a los hijos (y nieto fue esperado por hijo1)
    waitpid(pid_h1, NULL, 0);
    waitpid(pid_h2, NULL, 0);

    printf("Resultados (padre):\n");
    printf("  Suma A (archivo00): %ld\n", sumaA_rec);
    printf("  Suma B (archivo01): %ld\n", sumaB_rec);
    printf("  Suma TOTAL (A + B): %ld\n", total_rec);

    free(a0); free(a1);
    return 0;
}
