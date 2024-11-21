/**
 * @file main.c
 * @brief Entry point of the system
 */

#include "expose_metrics.h"

#include <cjson/cJSON.h>
#include <stdbool.h>

void load_config(const char* filename);

/**
 * @brief Time to sleep between metric updates.
 */
int SLEEP_TIME = 1;

/**
 * \brief Main function of the application.
 * \param argc Number of command-line arguments.
 * \param argv Array of command-line arguments.
 * \return Exit status of the application.
 */
int main(int argc, char* argv[])
{
    load_config("config.json");

    // Creamos un hilo para exponer las métricas vía HTTP
    pthread_t tid;
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != 0) // Successfull thread creation returns 0
    {
        fprintf(stderr, "Error al crear el hilo del servidor HTTP\n");
        return EXIT_FAILURE;
    }

    init_metrics(); // Initialize mutex and metrics

    // Bucle principal para actualizar las métricas cada segundo
    while (true) {
        if (metrics_state.cpu) {
            update_cpu_gauge();
        }
        if (metrics_state.memory) {
            update_memory_gauge();
        }
        if (metrics_state.disk) {
            update_disk_gauge();
        }
        if (metrics_state.network) {
            update_net_gauge();
        }
        sleep(SLEEP_TIME);
    }


    return EXIT_SUCCESS;
}

void load_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir config.json");
        return;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = malloc(length + 1);
    fread(data, 1, length, file);
    fclose(file);
    data[length] = '\0';

    cJSON* config = cJSON_Parse(data);
    free(data);

    if (!config) {
        fprintf(stderr, "Error al parsear config.json: %s\n", cJSON_GetErrorPtr());
        return;
    }

    // Leer el intervalo de muestreo
    cJSON* interval = cJSON_GetObjectItem(config, "sampling_interval");
    if (cJSON_IsNumber(interval)) {
        SLEEP_TIME = interval->valueint;
    }

    // Leer las métricas habilitadas
    cJSON* enabled_metrics = cJSON_GetObjectItem(config, "enabled_metrics");
    if (cJSON_IsArray(enabled_metrics)) {
        // Inicializar estado de las métricas en falso
        metrics_state.cpu = false;
        metrics_state.memory = false;
        metrics_state.disk = false;
        metrics_state.network = false;

        cJSON* metric = NULL;
        cJSON_ArrayForEach(metric, enabled_metrics) {
            if (cJSON_IsString(metric)) {
                if (strcmp(metric->valuestring, "cpu") == 0) {
                    metrics_state.cpu = true;
                } else if (strcmp(metric->valuestring, "memory") == 0) {
                    metrics_state.memory = true;
                } else if (strcmp(metric->valuestring, "disk") == 0) {
                    metrics_state.disk = true;
                } else if (strcmp(metric->valuestring, "network") == 0) {
                    metrics_state.network = true;
                }
            }
        }
    }

    cJSON_Delete(config);
}
