#ifndef METRICS_H
#define METRICS_H
/**
 * @file metrics.h
 * @brief Funciones para obtener el uso de CPU y memoria desde el sistema de archivos /proc.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @def BUFFER_SIZE
 * @brief Size of the buffer used for reading files.
 */
#define BUFFER_SIZE 256

/**
 * @def SHORT_BUFFER_SIZE
 * @brief Size of the buffer used for reading short strings.
 */
#define SHORT_BUFFER_SIZE 32

/**
 * @struct DiskStats
 * @brief Structure to hold disk statistics.
 */
typedef struct
{
    double rps; /**< Read operations per second. */
    double wps; /**< Write operations per second. */
} DiskStats;

/**
 * @struct NetStats
 * @brief Structure to hold network statistics.
 */
typedef struct
{
    double rec_bytesps; /**< Received bytes per second. */
    double sen_bytesps; /**< Sent bytes per second. */
} NetStats;

/**
 * @struct CpuStats
 * @brief Structure to hold CPU statistics.
 */
typedef struct
{
    double cpu_usage;        /**< CPU usage percentage. */
    int procs_running;       /**< Number of running processes. */
    unsigned long long ctxt; /**< Number of context switches. */
} CpuStats;

/**
 * @struct MetricsState
 * @brief Structure to hold the state of the metrics.
 */
typedef struct
{
    bool cpu;
    bool memory;
    bool disk;
    bool network;
} MetricsState;

extern MetricsState metrics_state;

/**
 * @brief Obtiene el porcentaje de uso de memoria desde /proc/meminfo.
 *
 * Lee los valores de memoria total y disponible desde /proc/meminfo y calcula
 * el porcentaje de uso de memoria.
 *
 * @return Uso de memoria como porcentaje (0.0 a 100.0), o -1.0 en caso de error.
 */
double get_memory_usage();

/**
 * @brief Calculates the CPU usage percentage, reads the procs_running and ctxt from /proc/stat
 *
 * Reads the CPU usage values from /proc/stat and calculates the CPU usage percentage, also reads the
 * number of running processes and context switches from /proc/stat
 *
 * @return A CpuStats struct with the CPU usage percentage, the number of running processes and context switches,
 * -1.0 in case of error
 */
CpuStats get_cpu_stats();

/**
 * @brief Calculates the number of read and write operations per second
 *
 * Reads the number of read and write operations from /proc/diskstats and the time
 * from proc/uptime to calculate the number of read and write operations per second
 *
 * @return A DiskStats struct with the number of read and write operations per second, -1.0 in case of error
 */
DiskStats get_disk_stats();

/**
 * @brief Calculates the number of kilobytes sent and received per second
 *
 * Reads the number of bytes sent and received from /proc/net/dev and the time
 * from proc/uptime to calculate the number of kilobytes sent and received per second
 *
 * @return A NetStats struct with the number of kilobytes sent and received per second, -1.0 in case of error
 */
NetStats get_net_stats();

#endif // METRICS_H
