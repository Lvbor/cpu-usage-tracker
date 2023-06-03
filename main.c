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
    unsigned long user; // Time used for user tasks
    unsigned long nice; // Time used for "nice" tasks
    unsigned long system; // Time used for system tasks
    unsigned long idle; // Processor idle time
} CPU_Data_t;

// A structure that stores data for each thread
typedef struct {
    pthread_t thread_id;
    int core_id;
    CPU_Data_t prev_data;
    CPU_Data_t curr_data;
} Thread_Data_t;

// Reader thread
void* readerThread(void* arg) {
    FILE* file = fopen("/proc/stat", "r"); // Opening /proc/stat (read)
    // Checks for NULL exception
    if (file == NULL) {
        perror("Failed to open /proc/stat");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_BUFFER]; 
    
    Thread_Data_t* threadData = (Thread_Data_t*)arg;

    while(TRUE) {
        fseek(file, 0, SEEK_SET); // Sets the position of the data read/write cursor to position 0 counting from the beginning of the file

        // Checks for NULL exception
        if (fgets(buffer, sizeof(buffer), file) == NULL) {
            perror("Failed to read /proc/stat");
            exit(EXIT_FAILURE);
        }

        // Checks that the beginning of the line read from the /proc/stat is equal to "cpu" and checks the CPU core ID
        if (strncmp(buffer, "cpu", 3) == 0 && sscanf(buffer + 3, "%d", &threadData->core_id) == 1) {
            // Checks for NULL exception
            if (sscanf(buffer + 3, "%*d %lu %lu %lu %lu", &threadData->curr_data.user, &threadData->curr_data.nice,
                       &threadData->curr_data.system, &threadData->curr_data.idle) != 4) {
                fprintf(stderr, "Failed to parse CPU data\n");
                exit(EXIT_FAILURE);
            }
        }

        // Checks whether the data has been correctly retrieved from the /proc/stat 
        printf("Core ID: %d, User: %lu, Nice: %lu, System: %lu, Idle: %lu\n",
        threadData->core_id, threadData->curr_data.user, threadData->curr_data.nice,
        threadData->curr_data.system, threadData->curr_data.idle);
        
        sleep(1);
    }
    fclose(file); // Closing /proc/stat
    return NULL;
}

int main() {
    Thread_Data_t threads[NUM_CORES];
    
    // Reader thread init
    for (int i = 0; i < NUM_CORES; i++) {
        threads[i].core_id = i;
        pthread_create(&threads[i].thread_id, NULL, readerThread, &threads[i]);
    }

    // Waiting for threads to end
    for (int i = 0; i < NUM_CORES; i++) {
        pthread_join(threads[i].thread_id, NULL);
    }

    return 0;
}