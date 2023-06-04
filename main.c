// Headers
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Defines
#define NUM_CORES 4
#define MAX_BUFFER 256
#define TRUE 1
#define WATCHDOG_TIMEOUT 2

// A structure that stores data for each core
typedef struct {
    unsigned long user;    // Time used for user tasks
    unsigned long nice;    // Time used for "nice" tasks
    unsigned long system;  // Time used for system tasks
    unsigned long idle;    // Processor idle time
    unsigned long irq;     // Time used for servicing interrupts
    unsigned long softirq; // Time used for servicing softirqs
    unsigned long steal;   // Stolen time (virtualization) [OracleVM]
} CPU_Data_t;

// A structure that stores data for each thread
#pragma pack(1) // Disable padding
typedef struct {
    CPU_Data_t prev_data;
    CPU_Data_t curr_data;
    pthread_t reader_thread_id;
    pthread_t analyzer_thread_id;
    __int32_t thread_index;
    bool is_alive; // Thread status for watchdog thread
} Thread_Data_t;
#pragma pack(4) // Reset to previous alignment rule (enable padding)

static float printData[NUM_CORES]; // Shared data structure for CPU usage data
static pthread_mutex_t printDataLock = PTHREAD_MUTEX_INITIALIZER; // Definition of printDataLock (mutex)

// Reader thread
static void *readerThread(void *arg) {
    Thread_Data_t *threadData = (Thread_Data_t *)arg;
    char buffer[MAX_BUFFER];

    while (TRUE) {
        // Create the file pointer for each thread
        FILE *file = fopen("/proc/stat", "r"); // Opening /proc/stat (read)
        // Checks for NULL exception
        if (file == NULL) {
            perror("Failed to open /proc/stat");
            exit(EXIT_FAILURE);
        }

        // Seek to the beginning of the file 
        fseek(file, 0, SEEK_SET);

        // Read the overall CPU usage data
        if (fgets(buffer, sizeof(buffer), file) == NULL) {
            perror("Failed to read /proc/stat");
            exit(EXIT_FAILURE);
        }

        // Parse the CPU usage data
        if (sscanf(buffer, "%*s %lu %lu %lu %lu %lu %lu %lu",
                   &(threadData->curr_data.user),
                   &(threadData->curr_data.nice),
                   &(threadData->curr_data.system),
                   &(threadData->curr_data.idle),
                   &(threadData->curr_data.irq),
                   &(threadData->curr_data.softirq),
                   &(threadData->curr_data.steal)) != 7) {
            fprintf(stderr, "Failed to parse CPU data\n");
            exit(EXIT_FAILURE);
        }

        // Close the file pointer
        fclose(file);

        // Sleep for 1s
        sleep(1);

        // Set the thread as alive
        threadData->is_alive = true;
    }

    return NULL;
}

static void *analyzerThread(void *arg) {
    Thread_Data_t *threadData = (Thread_Data_t *)arg;

    threadData->prev_data = threadData->curr_data; // Initialize prev_data

    while (1) {
        // Getting the values needed to calculate CPU usage
        unsigned long prev_idle = threadData->prev_data.idle +
                                  threadData->prev_data.irq +
                                  threadData->prev_data.softirq;
        unsigned long curr_idle = threadData->curr_data.idle +
                                  threadData->curr_data.irq +
                                  threadData->curr_data.softirq;

        unsigned long prev_non_idle = threadData->prev_data.user +
                                      threadData->prev_data.nice +
                                      threadData->prev_data.system +
                                      threadData->prev_data.steal;
        unsigned long curr_non_idle = threadData->curr_data.user +
                                      threadData->curr_data.nice +
                                      threadData->curr_data.system +
                                      threadData->curr_data.steal;

        unsigned long prev_total = prev_idle + prev_non_idle;
        unsigned long curr_total = curr_idle + curr_non_idle;

        unsigned long total_diff = curr_total - prev_total;
        unsigned long idle_diff = curr_idle - prev_idle;

        // Calculating the percentage of CPU usage
        float cpu_usage;
        if (total_diff != 0) {
            cpu_usage = (float)(total_diff - idle_diff) * 100.0f / (float)total_diff;
        } else {
            cpu_usage = 0.0f;
        }

        // Acquire the lock to safely access the shared printData structure
        pthread_mutex_lock(&printDataLock);
        printData[threadData->thread_index] = cpu_usage;
        pthread_mutex_unlock(&printDataLock);

        // Replace previous data with current data
        threadData->prev_data = threadData->curr_data;

        // Sleep for 1s
        sleep(1);

        // Set the thread as alive
        threadData->is_alive = true;
    }

    return NULL;
}

static void *printerThread() {
    
    while (1) {
        float total_cpu_usage = 0.0f;
        float average_cpu_usage = 0.0f;
        // Acquire the lock to safely access the shared printData structure
        pthread_mutex_lock(&printDataLock);

        // Calculate the total CPU usage
        for (int i = 0; i < NUM_CORES; i++) {
            total_cpu_usage += printData[i];
        }

        // Calculate the average CPU usage
        average_cpu_usage = total_cpu_usage / NUM_CORES;

        printf("Average CPU usage: %.2f%%\n", (double)average_cpu_usage);

        pthread_mutex_unlock(&printDataLock);

        // Sleep for 1s
        sleep(1);
    }

    return NULL;
}

static void *watchdogThread(void *arg) {
    Thread_Data_t *threads = (Thread_Data_t *)arg;

    while (1) {
        sleep(WATCHDOG_TIMEOUT);
        for (int i = 0; i < NUM_CORES; i++) {
            if (!threads[i].is_alive) {
                fprintf(stderr, "Thread %d did not report in time. Exiting...\n", i);
                exit(EXIT_FAILURE);
            } else {
                threads[i].is_alive = false;
            }
        }
    }

    return NULL;
}

int main() {
    Thread_Data_t threads[NUM_CORES];
    pthread_t printer_thread, watchdog_thread;

    // Reader and analyzer thread initialization
    for (int i = 0; i < NUM_CORES; i++) {
        threads[i].thread_index = i;
        threads[i].is_alive = false;
        pthread_create(&threads[i].reader_thread_id, NULL, readerThread, &threads[i]);
        pthread_create(&threads[i].analyzer_thread_id, NULL, analyzerThread, &threads[i]);
    }

    // Printer thread initialization
    pthread_create(&printer_thread, NULL, printerThread, NULL);

    // Watchdog thread initialization
    pthread_create(&watchdog_thread, NULL, watchdogThread, threads);

    for (int i = 0; i < NUM_CORES; i++) {
        pthread_join(threads[i].reader_thread_id, NULL);
        pthread_join(threads[i].analyzer_thread_id, NULL);
    }

    pthread_join(printer_thread, NULL);
    pthread_join(watchdog_thread, NULL);

    return 0;
}
