#include "metrics.h"

MetricsState metrics_state = {true, true, true, true};

double get_memory_usage()
{
    FILE* fp;
    char buffer[BUFFER_SIZE];
    unsigned long long total_mem = 0, free_mem = 0;

    // Abrir el archivo /proc/meminfo
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/meminfo");
        return -1.0;
    }

    // Leer los valores de memoria total y disponible
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "MemTotal: %llu kB", &total_mem) == 1)
        {
            continue; // MemTotal encontrado
        }
        if (sscanf(buffer, "MemAvailable: %llu kB", &free_mem) == 1)
        {
            break; // MemAvailable encontrado, podemos dejar de leer
        }
    }

    fclose(fp);

    // Verificar si se encontraron ambos valores
    if (total_mem == 0 || free_mem == 0)
    {
        fprintf(stderr, "Error al leer la información de memoria desde /proc/meminfo\n");
        return -1.0;
    }

    // Calcular el porcentaje de uso de memoria
    double used_mem = total_mem - free_mem;
    double mem_usage_percent = (used_mem / total_mem) * 100.0;

    return mem_usage_percent;
}

CpuStats get_cpu_stats()
{
    static unsigned long long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0, prev_iowait = 0,
                              prev_irq = 0, prev_softirq = 0, prev_steal = 0;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    unsigned long long totald, idled;
    CpuStats cpu_stats = {-1.0, -1, -1}; // Initialize to -1.0, -1, -1 in case of error

    // Abrir el archivo /proc/stat
    FILE* fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/stat");
        return cpu_stats;
    }

    char buffer[BUFFER_SIZE * 4];
    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        perror("Error al leer /proc/stat");
        fclose(fp);
        return cpu_stats;
    }

    // Analizar los valores de tiempo de CPU
    int ret = sscanf(buffer, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait,
                     &irq, &softirq, &steal);
    if (ret < 8)
    {
        fprintf(stderr, "Error al parsear /proc/stat\n");
        return cpu_stats;
    }

    // Loop through the file to find the context switches and running processes
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "ctxt %llu", &cpu_stats.ctxt) == 1)
        {
            continue; // context switches found
        }
        if (sscanf(buffer, "procs_running %d", &cpu_stats.procs_running) == 1)
        {
            break; // Procs running found, we can stop reading
        }
    }
    fclose(fp); // Close the file

    // Calcular las diferencias entre las lecturas actuales y anteriores
    unsigned long long prev_idle_total = prev_idle + prev_iowait;
    unsigned long long idle_total = idle + iowait;

    unsigned long long prev_non_idle = prev_user + prev_nice + prev_system + prev_irq + prev_softirq + prev_steal;
    unsigned long long non_idle = user + nice + system + irq + softirq + steal;

    unsigned long long prev_total = prev_idle_total + prev_non_idle;
    unsigned long long total = idle_total + non_idle;

    totald = total - prev_total;
    idled = idle_total - prev_idle_total;

    if (totald == 0)
    {
        fprintf(stderr, "Totald es cero, no se puede calcular el uso de CPU!\n");
        return cpu_stats;
    }

    // Calcular el porcentaje de uso de CPU
    cpu_stats.cpu_usage = ((double)(totald - idled) / totald) * 100.0;

    // Actualizar los valores anteriores para la siguiente lectura
    prev_user = user;
    prev_nice = nice;
    prev_system = system;
    prev_idle = idle;
    prev_iowait = iowait;
    prev_irq = irq;
    prev_softirq = softirq;
    prev_steal = steal;

    return cpu_stats; // Return the struct with the CPU usage percentage, the number of running processes and context
                      // switches
}

DiskStats get_disk_stats()
{
    FILE *fp, *fp_uptime;                  // File pointers
    char buffer[BUFFER_SIZE];              // Buffer to read the file
    char device_name[SHORT_BUFFER_SIZE];   // To store the device name
    static DiskStats stats = {-1.0, -1.0}; // Initialize to -1.0, -1.0 in case of error
    static double prev_time = 0;
    static unsigned long long prev_reads_completed = 0, prev_writes_completed = 0;
    double time, time_diff;
    unsigned long long reads_completed, writes_completed;

    // Opens the /proc/diskstats file, and checks if succeeded
    fp = fopen("/proc/diskstats", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/diskstats");
        return stats;
    }
    // Open the /proc/uptime file, and checks if succeeded
    fp_uptime = fopen("/proc/uptime", "r");
    if (fp_uptime == NULL)
    {
        perror("Error opening /proc/uptime");
        return stats;
    }

    // Reads the time from /proc/uptime
    fscanf(fp_uptime, "%lf", &time);
    fclose(fp_uptime); // Close the uptime file

    // Clculates the time difference
    time_diff = time - prev_time;
    prev_time = time; // Updates the previous time

    // Loop through the file to find the device
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "%*u %*u %s %llu %*u %*u %*u %llu", device_name, &reads_completed, &writes_completed) == 3)
        {
            if (strcmp(device_name, "nvme0n1") == 0)
            {
                break; // Device found, no need to keep reading
            }
        }
    }
    fclose(fp); // Close the diskstats file

    // Calculates the number of read and write operations per second
    stats.rps = (double)(reads_completed - prev_reads_completed) / time_diff;
    stats.wps = (double)(writes_completed - prev_writes_completed) / time_diff;
    prev_reads_completed = reads_completed;
    prev_writes_completed = writes_completed;

    return stats; // Return the struct with the number of read and write operations per second
}

NetStats get_net_stats()
{
    FILE *fp, *fp_uptime;                 // File pointers
    char buffer[BUFFER_SIZE];             // Buffer to read the file
    static NetStats stats = {-1.0, -1.0}; // Initialize to -1.0, -1.0 in case of error
    static double prev_time = 0, time, time_diff;
    static unsigned long long prev_rec_bytes = 0, prev_sen_bytes = 0;
    unsigned long long rec_bytes, sen_bytes, diff_rec_bytes, diff_sen_bytes;

    // We open the /proc/net/dev file, and check if we succeeded
    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/net/dev");
        return stats;
    }
    // We open the /proc/uptime file, and check if we succeeded
    fp_uptime = fopen("/proc/uptime", "r");
    if (fp_uptime == NULL)
    {
        perror("Error opening /proc/uptime");
        return stats;
    }

    // We read the time from /proc/uptime
    fscanf(fp_uptime, "%lf", &time);
    fclose(fp_uptime); // Close the uptime file

    // We calculate the time difference
    time_diff = time - prev_time;
    prev_time = time; // Update the previous time

    // Loop through the file to find the interface
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "  wlo1: %llu %*u %*u %*u %*u %*u %*u %*u %llu", &rec_bytes, &sen_bytes) == 2)
        {
            break; // Interface found, no need to keep reading
        }
    }
    fclose(fp); // Close the net/dev file

    // We calculate the difference in bytes sent and received
    diff_rec_bytes = rec_bytes - prev_rec_bytes;
    prev_rec_bytes = rec_bytes;
    diff_sen_bytes = sen_bytes - prev_sen_bytes;
    prev_sen_bytes = sen_bytes;

    // We calculate the number of kilobytes sent and received per second
    stats.rec_bytesps = (double)diff_rec_bytes / time_diff;
    stats.sen_bytesps = (double)diff_sen_bytes / time_diff;

    return stats;
}
