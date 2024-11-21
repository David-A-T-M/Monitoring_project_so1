#include "expose_metrics.h"
#include <metrics.h>

/** Mutex para sincronización de hilos */
pthread_mutex_t lock;

/** CPU usage metric */
static prom_gauge_t* cpu_usage_metric;

/** Context switches metric */
static prom_gauge_t* context_switches_metric;

/** Running processes metric */
static prom_gauge_t* procs_running_metric;

/** Métrica de Prometheus para el uso de memoria */
static prom_gauge_t* memory_usage_metric;

/** Read operations per second metric */
static prom_gauge_t* disk_read_metric;

/** Write operations per second metric */
static prom_gauge_t* disk_write_metric;

/** Received bytes per second metric */
static prom_gauge_t* net_rec_bytes_metric;

/** Sent bytes per second metric */
static prom_gauge_t* net_sen_bytes_metric;

void update_cpu_gauge()
{
    CpuStats cpu_stats =
        get_cpu_stats(); // Get the CPU usage percentage, the number of running processes and context switches
    if (cpu_stats.cpu_usage >= 0 && cpu_stats.procs_running >= 0 && cpu_stats.ctxt >= 0)
    {
        pthread_mutex_lock(&lock);                                           // Lock the mutex
        prom_gauge_set(cpu_usage_metric, cpu_stats.cpu_usage, NULL);         // Update the CPU usage
        prom_gauge_set(context_switches_metric, cpu_stats.ctxt, NULL);       // Update the number of context switches
        prom_gauge_set(procs_running_metric, cpu_stats.procs_running, NULL); // Update the number of running processes
        pthread_mutex_unlock(&lock);                                         // Unlock the mutex
    }
    else
    {
        fprintf(stderr, "Error obtaining CPU stats\n");
    }
}

void update_memory_gauge()
{
    double usage = get_memory_usage();
    if (usage >= 0)
    {
        pthread_mutex_lock(&lock);
        prom_gauge_set(memory_usage_metric, usage, NULL);
        pthread_mutex_unlock(&lock);
    }
    else
    {
        fprintf(stderr, "Error al obtener el uso de memoria\n");
    }
}

void update_disk_gauge()
{
    DiskStats disk_stats = get_disk_stats(); // Get the number of read and write operations per second
    if (disk_stats.rps >= 0 && disk_stats.wps >= 0)
    {
        pthread_mutex_lock(&lock);                               // Lock the mutex
        prom_gauge_set(disk_read_metric, disk_stats.rps, NULL);  // Update the number of read operations per second
        prom_gauge_set(disk_write_metric, disk_stats.wps, NULL); // Update the number of write operations per second
        pthread_mutex_unlock(&lock);                             // Unlock the mutex
    }
    else
    {
        fprintf(stderr, "Error obtaining disk stats\n");
    }
}

void update_net_gauge()
{
    NetStats net_stats = get_net_stats(); // Get the number of received and sent bytes per second
    if (net_stats.rec_bytesps >= 0 && net_stats.sen_bytesps >= 0)
    {
        pthread_mutex_lock(&lock); // Lock the mutex
        prom_gauge_set(net_rec_bytes_metric, net_stats.rec_bytesps,
                       NULL); // Update the number of received bytes per second
        prom_gauge_set(net_sen_bytes_metric, net_stats.sen_bytesps, NULL); // Update the number of sent bytes per second
        pthread_mutex_unlock(&lock);                                       // Unlock the mutex
    }
    else
    {
        fprintf(stderr, "Error obtaining network stats\n");
    }
}

void* expose_metrics(void* arg)
{
    (void)arg; // Argumento no utilizado

    // Aseguramos que el manejador HTTP esté adjunto al registro por defecto
    promhttp_set_active_collector_registry(NULL);

    // Iniciamos el servidor HTTP en el puerto 8000
    struct MHD_Daemon* daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, 8000, NULL, NULL);
    if (daemon == NULL)
    {
        fprintf(stderr, "Error al iniciar el servidor HTTP\n");
        return NULL;
    }

    // Mantenemos el servidor en ejecución
    while (1)
    {
        sleep(1);
    }

    // Nunca debería llegar aquí
    MHD_stop_daemon(daemon);
    return NULL;
}

void init_metrics()
{
    // Inicializamos el mutex
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        fprintf(stderr, "Error al inicializar el mutex\n");
        return EXIT_FAILURE;
    }

    // Inicializamos el registro de coleccionistas de Prometheus
    if (prom_collector_registry_default_init() != 0)
    {
        fprintf(stderr, "Error al inicializar el registro de Prometheus\n");
        return EXIT_FAILURE;
    }

    // Creamos la métrica para el uso de memoria
    memory_usage_metric = prom_gauge_new("memory_usage_percentage", "Porcentaje de uso de memoria", 0, NULL);
    if (memory_usage_metric == NULL)
    {
        fprintf(stderr, "Error al crear la métrica de uso de memoria\n");
        return EXIT_FAILURE;
    }

    if (prom_collector_registry_must_register_metric(memory_usage_metric) == NULL)
    {
        fprintf(stderr, "Error al registrar la métrica de uso de memoria\n");
    }

    // Creates and registers the metric for disk write operations
    disk_read_metric = prom_collector_registry_must_register_metric(
        prom_gauge_new("disk_read_operations", "Number of reading operations per second", 0, NULL));

    // Creates and registers the metric for disk write operations
    disk_write_metric = prom_collector_registry_must_register_metric(
        prom_gauge_new("disk_write_operations", "Number of writing operations per second", 0, NULL));

    // Creates and registers the metric for cpu usage
    cpu_usage_metric = prom_collector_registry_must_register_metric(
        prom_gauge_new("cpu_usage_percentage", "Percentage of CPU usage", 0, NULL));

    // Creates and registers the metric for context switches
    context_switches_metric = prom_collector_registry_must_register_metric(
        prom_gauge_new("context_switches", "Number of context switches", 0, NULL));

    // Creates and registers the metric for running processes
    procs_running_metric = prom_collector_registry_must_register_metric(
        prom_gauge_new("procs_running", "Number of running processes", 0, NULL));

    // Creates and registers the metric for received bytes per second
    net_rec_bytes_metric = prom_collector_registry_must_register_metric(
        prom_gauge_new("net_received_bytes", "Number of received bytes per second", 0, NULL));

    // Creates and registers the metric for sent bytes per second
    net_sen_bytes_metric = prom_collector_registry_must_register_metric(
        prom_gauge_new("net_sent_bytes", "Number of sent bytes per second", 0, NULL));
}

void destroy_mutex()
{
    pthread_mutex_destroy(&lock);
}
