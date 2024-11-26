#ifndef EXPOSE_METRICS_H
#define EXPOSE_METRICS_H
/**
 * @file expose_metrics.h
 * @brief Programa para leer el uso de CPU y memoria y exponerlos como métricas de Prometheus.
 */

#include "metrics.h"
#include <errno.h>
#include <prom.h>
#include <promhttp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // Para sleep

extern MemoryStats memory_stats;
extern CpuStats cpu_stats;
extern DiskStats disk_stats;
extern NetStats net_stats;

/** Mutex para sincronización de hilos */
extern pthread_mutex_t lock;

/**
 * \def BUFFER_SIZE
 * \brief Size of the buffer used for reading files.
 */
#define BUFFER_SIZE 1024

/**
 * @brief Updates the CPU usage, context switches and running processes metrics
 */
void update_cpu_gauge();

/**
 * @brief Actualiza la métrica de uso de memoria.
 */
void update_memory_gauge();

/**
 * @brief Updates the disk read and write operations per second metric
 */
void update_disk_gauge();

/**
 * @brief Updates the network received and sent bytes per second metric
 */
void update_net_gauge();

/**
 * @brief Función del hilo para exponer las métricas vía HTTP en el puerto 8000.
 * @param arg Argumento no utilizado.
 * @return NULL
 */
void* expose_metrics(void* arg);

/**
 * @brief Inicializar mutex y métricas.
 */
void init_metrics();

/**
 * @brief Destructor de mutex
 */
void destroy_mutex();

#endif // EXPOSE_METRICS_H
