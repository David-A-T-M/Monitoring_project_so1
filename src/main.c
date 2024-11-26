/**
 * @file main.c
 * @brief Entry point of the system
 */

#include "expose_metrics.h"
#include <cjson/cJSON.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FIFO_PATH "/tmp/monitor_fifo"
volatile sig_atomic_t write_fifo_flag = 0;

void signal_handler(int signo);
void write_active_metrics_to_fifo();
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
    load_config("./config.json");

    if (access(FIFO_PATH, F_OK) == -1) {
        if (mkfifo(FIFO_PATH, 0666) == -1) {
            perror("Error creating FIFO");
            return EXIT_FAILURE;
        }
    }

    // Configurar manejador de señal
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Error setting up signal handler");
        return EXIT_FAILURE;
    }

    // Creamos un hilo para exponer las métricas vía HTTP
    init_metrics(); // Initialize mutex and metrics

    pthread_t tid;
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != 0) // Successfull thread creation returns 0
    {
        fprintf(stderr, "Error al crear el hilo del servidor HTTP\n");
        unlink(FIFO_PATH); // Limpiar la FIFO en caso de error
        return EXIT_FAILURE;
    }

    // Bucle principal para actualizar las métricas cada segundo
    while (true)
    {
        update_cpu_gauge();
        update_memory_gauge();
        update_disk_gauge();
        update_net_gauge();

        if (write_fifo_flag) {
            write_active_metrics_to_fifo();
            write_fifo_flag = 0;
        }

        sleep(SLEEP_TIME);
    }

    unlink(FIFO_PATH); // Eliminar la FIFO al salir
    return EXIT_SUCCESS;
}

void load_config(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
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

    if (!config)
    {
        fprintf(stderr, "Error al parsear config.json: %s\n", cJSON_GetErrorPtr());
        return;
    }

    // Leer el intervalo de muestreo
    cJSON* interval = cJSON_GetObjectItem(config, "sampling_interval");
    if (cJSON_IsNumber(interval))
    {
        SLEEP_TIME = interval->valueint;
    }

    // Leer las métricas habilitadas
    cJSON* enabled_metrics = cJSON_GetObjectItem(config, "enabled_metrics");
    if (cJSON_IsArray(enabled_metrics))
    {
        // Inicializar estado de las métricas en falso
        metrics_state.cpu = false;
        metrics_state.memory = false;
        metrics_state.disk = false;
        metrics_state.network = false;

        cJSON* metric = NULL;
        cJSON_ArrayForEach(metric, enabled_metrics)
        {
            if (cJSON_IsString(metric))
            {
                if (strcmp(metric->valuestring, "cpu") == 0)
                {
                    metrics_state.cpu = true;
                }
                else if (strcmp(metric->valuestring, "memory") == 0)
                {
                    metrics_state.memory = true;
                }
                else if (strcmp(metric->valuestring, "disk") == 0)
                {
                    metrics_state.disk = true;
                }
                else if (strcmp(metric->valuestring, "network") == 0)
                {
                    metrics_state.network = true;
                }
            }
        }
    }

    cJSON_Delete(config);
}

void signal_handler(int signo) {
    write_fifo_flag = 1;
}

void write_active_metrics_to_fifo()
{
    char buffer[512]; // Suficiente para todas las métricas activas
    int offset = 0;

    // Verificar qué métricas están activas y agregar al buffer
    if (metrics_state.cpu)
    {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                           "CPU - Usage: %.2f, Procs running: %d, Ctx switches: %llu\n", cpu_stats.cpu_usage,
                           cpu_stats.procs_running, cpu_stats.ctxt);
    }
    if (metrics_state.memory)
    {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Memory - Usage: %.2f%%\n", memory_stats.usage);
    }
    if (metrics_state.disk)
    {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Disk - RPS: %.2f, WPS: %.2f\n", disk_stats.rps,
                           disk_stats.wps);
    }
    if (metrics_state.network)
    {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Network - RX: %.2f, TX: %.2f\n",
                           net_stats.rec_bytesps, net_stats.sen_bytesps);
    }

    // Escribir en la FIFO
    int fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (fd != -1)
    {
        write(fd, buffer, offset);
        close(fd);
    }
}
