#include "reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_BUFFER 256

extern volatile sig_atomic_t flagSigTerm;

// Reader thread
void *readerThread(void *arg)
{
    Thread_Data_t *threadData = (Thread_Data_t *)arg;
    char buffer[MAX_BUFFER];

    while (!flagSigTerm)
    {
        // Create the file pointer for each thread
        FILE *file = fopen("/proc/stat", "r"); // Opening /proc/stat (read)
        // Checks for NULL exception
        if (file == NULL)
        {
            perror("Failed to open /proc/stat");
            exit(EXIT_FAILURE);
        }

        // Seek to the beginning of the file
        fseek(file, 0, SEEK_SET);

        // Read the overall CPU usage data
        if (fgets(buffer, sizeof(buffer), file) == NULL)
        {
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
                   &(threadData->curr_data.steal)) != 7)
        {
            fprintf(stderr, "Failed to parse CPU data\n");
            exit(EXIT_FAILURE);
        }

        // Close the file pointer
        fclose(file);

        // Sleep for 1s
        sleep(1);

        // Set the thread as alive
        threadData->is_alive_reader = true;
    }

    return NULL;
}