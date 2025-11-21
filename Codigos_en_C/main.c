#include "funciones_dinamica.h"
#include "funciones_red.h"
#include "random.h"

extern int xp[];
extern int yp[];
extern int zp[];
extern int xm[];
extern int ym[];
extern int zm[];

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>

#define NVAR 11        // Número de variables VAR1 ... VAR11
#define MAXLINE 1024   // Tamaño máximo de línea
#define MAXPATH 512    // Longitud máxima de ruta
#define EPS 1e-30      // Evita log(0) o división por cero


static void procesarArchivo(const char *filename,
                            long double suma[],
                            long double suma2[],
                            unsigned long long *nfilas_totales) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "No se pudo abrir: %s\n", filename);
        return;
    }

    char line[MAXLINE];
    double tiempo;
    double vals[NVAR];

    while (fgets(line, sizeof(line), fp)) {
        /* Leer tiempo (primera columna) */
        int off = 0;
        if (sscanf(line, "%lf%n", &tiempo, &off) != 1) continue;
        char *p = line + off;

        /* Leer NVAR valores (si faltan columnas, saltamos la línea) */
        int i;
        for (i = 0; i < NVAR; ++i) {
            int used = 0;
            if (sscanf(p, "%lf%n", &vals[i], &used) != 1) break;
            p += used;
        }
        if (i != NVAR) continue; /* línea incompleta -> ignorar */

        /* Acumular en long double */
        for (i = 0; i < NVAR; ++i) {
            long double v = (long double) vals[i];
            suma[i]  += v;
            suma2[i] += v * v;
        }
        (*nfilas_totales)++;
    }

    fclose(fp);
}

void calcularMediasGlobales(const char *carpetas[], int ncarpetas,
                            const char *nombreSalida) {
    long double suma[NVAR];
    long double suma2[NVAR];
    for (int i = 0; i < NVAR; ++i) { suma[i] = 0.0L; suma2[i] = 0.0L; }
    unsigned long long nfilas_totales = 0ULL;

    for (int c = 0; c < ncarpetas; ++c) {
        DIR *d = opendir(carpetas[c]);
        if (!d) {
            fprintf(stderr, "No se pudo abrir carpeta: %s\n", carpetas[c]);
            continue;
        }
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            /* coger ficheros que empiecen por "I_" y terminen en ".txt" */
            if (strncmp(e->d_name, "I_", 2) == 0 &&
                strstr(e->d_name, ".txt") != NULL) {
                char ruta[MAXPATH];
                snprintf(ruta, sizeof(ruta), "%s/%s", carpetas[c], e->d_name);
                procesarArchivo(ruta, suma, suma2, &nfilas_totales);
            }
        }
        closedir(d);
    }

    if (nfilas_totales == 0) {
        fprintf(stderr, "No se encontraron filas en los archivos.\n");
        return;
    }

    FILE *out = fopen(nombreSalida, "w");
    if (!out) { perror("Error creando salida"); return; }

    fprintf(out, "# MEDIA GLOBAL DE TODAS LAS VARIABLES (N_total = %llu)\n", nfilas_totales);
    fprintf(out, "# N\tmedia\t\terr_media\t\tlog(media)\t\terr_media/media\n");

    for (int i = 0; i < NVAR; ++i) {
        long double mean = suma[i] / (long double) nfilas_totales;
        long double mean2 = suma2[i] / (long double) nfilas_totales;
        long double var = mean2 - mean * mean;
        if (var < 0 && fabsl(var) < 1e-28L) var = 0.0L; /* corrección por redondeo */
        if (var < 0) {
            /* si es negativo grande, algo fue mal en lectura */
            fprintf(stderr, "Advertencia: var negativa para i=%d: %Le\n", i+1, var);
            var = 0.0L;
        }
        long double err = sqrtl(var);
        long double safe_mean = fabsl(mean) < EPS ? EPS : mean;
        long double logm = logl(safe_mean);
        long double err_rel = (fabsl(mean) < EPS) ? 0.0L : err / mean;

        /* %d -> índice, %.12Le -> long double en notación exponencial */
        fprintf(out, "%d\t%.12Le\t%.12Le\t%.12Le\t%.12Le\n",
                i + 1, mean, err, logm, err_rel);
    }

    fclose(out);
    printf("Resultados guardados en '%s' (N_total = %llu)\n", nombreSalida, nfilas_totales);
}



// -------------------------------------------------------------
// Calcula medias y errores de UN archivo y añade a SALIDA.txt
// -------------------------------------------------------------
void calcularMediasArchivo(const char *filename, const char *nombreSalida) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "❌ No se pudo abrir el archivo: %s\n", filename);
        return;
    }

    double suma[NVAR] = {0.0};
    double suma2[NVAR] = {0.0};
    unsigned long long nfilas = 0ULL;

    char line[MAXLINE];
    double tiempo, vals[NVAR];

    while (fgets(line, sizeof(line), fp)) {
        int offset = 0;
        if (sscanf(line, "%lf%n", &tiempo, &offset) != 1) continue;
        char *ptr = line + offset;

        int i;
        for (i = 0; i < NVAR; i++) {
            int used = 0;
            if (sscanf(ptr, "%lf%n", &vals[i], &used) != 1) break;
            ptr += used;
        }
        if (i != NVAR) continue;

        for (i = 0; i < NVAR; i++) {
            double v = vals[i];
            suma[i] += v;
            suma2[i] += v * v;
        }
        nfilas++;
    }
    fclose(fp);

    if (nfilas == 0) {
        fprintf(stderr, "⚠️ El archivo %s no contiene datos válidos.\n", filename);
        return;
    }

    FILE *salida = fopen(nombreSalida, "a");
    if (!salida) {
        perror("Error abriendo fichero de salida");
        return;
    }

    fprintf(salida, "----------- %s -----------\n", filename);
    fprintf(salida, "N_variable\tMedia_variable\tErr_variable\tLog(variable)\tErr_variable/Media_variable\n");

    for (int i = 0; i < NVAR; i++) {
        double media = suma[i] / (double)nfilas;
        double var = (suma2[i] / (double)nfilas) - media * media;
        if (var < 0 && fabs(var) < 1e-20) var = 0.0;
        if (var < 0) {
            fprintf(stderr, "⚠️ Varianza negativa en variable %d (%s)\n", i + 1, filename);
            var = 0.0;
        }
        double err = sqrt(var);
        double safe_media = fabs(media) < EPS ? EPS : media;
        double logm = log(safe_media);
        double err_rel = (fabs(media) < EPS) ? 0.0 : err / media;

        fprintf(salida, "%d\t%.8e\t%.8e\t%.8e\t%.8e\n",
                i + 1, media, err, logm, err_rel);
    }

    fprintf(salida, "\n");
    fclose(salida);

    printf("✅ Resultados añadidos a %s (filas: %llu)\n", nombreSalida, nfilas);
}

void procesarTodasLasCarpetas(const char *carpetas[], int ncarpetas, const char *nombreSalida) {
    // Limpiar el fichero de salida antes de empezar
    FILE *salida = fopen(nombreSalida, "w");
    if (salida) fclose(salida);

    for (int c = 0; c < ncarpetas; c++) {
        DIR *dir = opendir(carpetas[c]);
        if (!dir) {
            fprintf(stderr, "⚠️ No se pudo abrir carpeta: %s\n", carpetas[c]);
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Filtrar archivos que empiecen por "I_" y terminen en ".txt"
            if (strncmp(entry->d_name, "I_", 2) == 0 && strstr(entry->d_name, ".txt")) {
                char ruta[MAXPATH];
                snprintf(ruta, sizeof(ruta), "%s/%s", carpetas[c], entry->d_name);
                calcularMediasArchivo(ruta, nombreSalida);
            }
        }
        closedir(dir);
    }

    printf("✅ Todos los archivos procesados. Resultados en '%s'\n", nombreSalida);
}

void guardar_parametros(int k_ini, int N_sweps_entre_medidas,int N_medidas) {
    char* folder_param;
    #ifdef Na
    if (beta == 0.72) {
        folder_param   = "Resultados_simulacion/MAIN/0.72/PARAMETROS/NASIO";
    } else if (beta == 0.8) {
        folder_param   = "Resultados_simulacion/MAIN/0.8/PARAMETROS/NASIO";
    } else {
        printf("PON EL VALOR DE BETA QUE TOCA, AMIGO MIO\n");
    }
#elif defined(Ba)
    if (beta == 0.72) {
        folder_param   = "Resultados_simulacion/MAIN/0.72/PARAMETROS/BARRACHINA";
    } else if (beta == 0.8) {
        folder_param   = "Resultados_simulacion/MAIN/0.8/PARAMETROS/BARRACHINA";
    } else {
        printf("PON EL VALOR DE BETA QUE TOCA, AMIGO MIO\n");
    }
#elif defined(Jo)
    if (beta == 0.72) {
        folder_param   = "Resultados_simulacion/MAIN/0.72/PARAMETROS/JOEL";
    } else if (beta == 0.8) {
        folder_param   = "Resultados_simulacion/MAIN/0.8/PARAMETROS/JOEL";
    } else {
        printf("PON EL VALOR DE BETA QUE TOCA, AMIGO MIO\n");
    }
#else
    #error "Debes definir Na, Ba o Jo antes de compilar"
#endif
    char filename[256];
    int k = 0;
    FILE* ftest;

    // Buscar el índice k más pequeño cuyo archivo no exista
    while (1) {
        snprintf(filename, sizeof(filename), "%s/I_%d.txt", folder_param, k);
        ftest = fopen(filename, "r");
        if (ftest) {
            fclose(ftest);
            k++;
        } else {
            break;
        }
    }

    // Crear el archivo I_k.txt
    FILE* fparam = fopen(filename, "w");
    if (!fparam) {
        printf("No se pudo crear el archivo de parámetros %s\n", filename);
        return;
    }

    // Escribir los parámetros en el formato especificado
    fprintf(fparam, "L\t%d\n", L);
    fprintf(fparam, "J\t%f\n", J);
    fprintf(fparam, "beta\t%f\n", beta);
    fprintf(fparam, "configuracion_inicial: \tI_%d.txt\n",k_ini);

    fprintf(fparam, "N_sweps_entre_medidas\t%d\n", N_sweps_entre_medidas);
    fprintf(fparam, "N_medidas\t%d\n", N_medidas);

    fclose(fparam);
    printf("Archivo de parámetros creado: %s\n", filename);
}



#define MAX_LINE 256
#define MAX_N 1000

typedef struct {
    double media_sum;
    double err_sum;
    int count;
} Stats;

// Función que procesa salida.txt y genera grafica.txt
void generar_grafica(const char *input_file, const char *output_file) {
    FILE *f = fopen(input_file, "r");
    if (!f) {
        perror(input_file);
        return;
    }

    Stats datos[MAX_N] = {0};
    char linea[MAX_LINE];

    while (fgets(linea, MAX_LINE, f)) {
        // Ignorar encabezados o separadores
        if (linea[0] == '-' || strstr(linea, "N_variable") != NULL || linea[0] == '\n') {
            continue;
        }

        int n;
        double media, err, log_var, err_over_var;

        // Leer solo las 5 primeras columnas
        if (sscanf(linea, "%d %lf %lf %lf %lf", &n, &media, &err, &log_var, &err_over_var) == 5) {
            if (n >= MAX_N) {
                printf("Warning: N demasiado grande: %d\n", n);
                continue;
            }
            datos[n].media_sum += media;
            datos[n].err_sum += err;
            datos[n].count++;
        }
    }

    fclose(f);

    FILE *out = fopen(output_file, "w");
    if (!out) {
        perror(output_file);
        return;
    }

    fprintf(out, "N\tMEDIA\tERR_N\n");
    for (int i = 0; i < MAX_N; i++) {
        if (datos[i].count > 0) {
            double media_prom = datos[i].media_sum / datos[i].count;
            double err_prom = datos[i].err_sum / datos[i].count;
            fprintf(out, "%d\t%.8e\t%.8e\n", i, media_prom, err_prom);
        }
    }

    fclose(out);
    printf("Archivo '%s' generado correctamente.\n", output_file);
}

int main(){
    /*

    inicializa_PR((int)time(NULL));
    int s[3*L*L*L], plaquetas[3*L*L*L];
    double probabilidades[5];
    int N_sweps_entre_med=35*3;
    int N_medidas=50000;
    char input_file[512]; 


    for(int j=0;j<100;j++){
    int k_ini=(int)(fran()*20);
    sprintf(input_file, "Resultados_simulacion/MAIN/0.72/CONFIGURACIONES/I_%d.txt", k_ini);
    vector_cociente_prob(probabilidades);
    inicializa_vectores_de_vecinos();
    lee_configuracionInicial(s, input_file);
    dame_plaquetas(s, plaquetas);
    guardar_parametros(k_ini,N_sweps_entre_med,N_medidas);
    dinamica_metropolis_main(N_sweps_entre_med, N_medidas, probabilidades,s, plaquetas,input_file);
    }
    */
   /*
        const char *carpetas[] = {
        "Resultados_simulacion/MAIN/0.72/EVOLUCION/BARRACHINA",
        "Resultados_simulacion/MAIN/0.72/EVOLUCION/JOEL",
        "Resultados_simulacion/MAIN/0.72/EVOLUCION/JOEL_1",
        "Resultados_simulacion/MAIN/0.72/EVOLUCION/NASIO",
        "Resultados_simulacion/MAIN/0.72/EVOLUCION/NASIO_1"
    };
    int ncarpetas = sizeof(carpetas) / sizeof(carpetas[0]);

    procesarTodasLasCarpetas(carpetas, ncarpetas, "SALIDA.txt");
    */
   generar_grafica("SALIDA.txt","GRAFICA_PORFIN.txt");
    return 0;
}