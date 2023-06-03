// Headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Defines
#define NUM_CORES 4
#define MAX_BUFFER 256
#define TRUE 1

// A structure that stores data for each core
typedef struct {
    unsigned long user;   // Time used for user tasks
    unsigned long nice;   // Time used for "nice" tasks
    unsigned long system; // Time used for system tasks
    unsigned long idle;   // Processor idle time
} CPU_Data_t;

// A structure that stores data for each thread
typedef struct {
    pthread_t thread_id;
    int thread_index;
    CPU_Data_t prev_data;
    CPU_Data_t curr_data;
} Thread_Data_t;

// Reader thread
void *readerThread(void *arg) {
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
        if (sscanf(buffer, "%*s %lu %lu %lu %lu", &(threadData->curr_data.user), &(threadData->curr_data.nice),
                   &(threadData->curr_data.system), &(threadData->curr_data.idle)) != 4) {
            fprintf(stderr, "Failed to parse CPU data\n");
            exit(EXIT_FAILURE);
        }

        fclose(file); // Close the file pointer

        sleep(1); // Sleep for 1s
    }

    return NULL;
}

void *analyzerThread(void *arg) {
    Thread_Data_t *threadData = (Thread_Data_t *)arg;

    threadData->prev_data = threadData->curr_data; // Initialize prev_data

    while (1) {

        // Getting the values needed to calculate CPU usage
        unsigned long prev_idle = threadData->prev_data.idle;
        unsigned long curr_idle = threadData->curr_data.idle;

        unsigned long prev_non_idle = threadData->prev_data.user + threadData->prev_data.nice +
                                      threadData->prev_data.system;
        unsigned long curr_non_idle = threadData->curr_data.user + threadData->curr_data.nice +
                                      threadData->curr_data.system;

        unsigned long prev_total = prev_idle + prev_non_idle;
        unsigned long curr_total = curr_idle + curr_non_idle;

        unsigned long total_diff = curr_total - prev_total;
        unsigned long idle_diff = curr_idle - prev_idle;

        // Calculating the percentage of CPU usage
        float cpu_usage;
        if (total_diff != 0) {
            cpu_usage = (total_diff - idle_diff) * 100.0 / total_diff;
        } else {
            cpu_usage = 0.0;
        }

        // Test print
        printf("Thread index: %d, CPU usage: %.2f%%\n", threadData->thread_index, cpu_usage);

        // Replace previous data with current data
        threadData->prev_data = threadData->curr_data;

        sleep(1); // Sleep for 1s
    }

    return NULL;
}

int main() {
    Thread_Data_t threads[NUM_CORES];

    // Reader and analyzer thread initialization
    for (int i = 0; i < NUM_CORES; i++) {
        threads[i].thread_index = i;
        pthread_create(&threads[i].thread_id, NULL, readerThread, &threads[i]);
        pthread_create(&threads[i].thread_id, NULL, analyzerThread, &threads[i]);
    }

     // Waiting for threads to end
    for (int i = 0; i < NUM_CORES; i++) {
        pthread_join(threads[i].thread_id, NULL);
    }

    return 0;
}
