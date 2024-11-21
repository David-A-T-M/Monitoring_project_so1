/**
 * @file main.c
 * @brief Entry point of the system
 */

#include "expose_metrics.h"
#include <stdbool.h>

/**
 * \def SLEEP_TIME
 * \brief Time to sleep between metric updates.
 */
#define SLEEP_TIME 1

/**
 * \brief Main function of the application.
 * \param argc Number of command-line arguments.
 * \param argv Array of command-line arguments.
 * \return Exit status of the application.
 */
int main(int argc, char* argv[])
{

    // Creamos un hilo para exponer las métricas vía HTTP
    pthread_t tid;
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != 0) // Successfull thread creation returns 0
    {
        fprintf(stderr, "Error al crear el hilo del servidor HTTP\n");
        return EXIT_FAILURE;
    }

    init_metrics(); // Initialize mutex and metrics

    // Bucle principal para actualizar las métricas cada segundo
    while (true)
    {
        update_net_gauge();
        update_cpu_gauge();
        update_disk_gauge();
        update_memory_gauge();
        sleep(SLEEP_TIME);
    }

    return EXIT_SUCCESS;
}
