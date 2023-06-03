// Headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Defines
#define NUM_CORES 4

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

int main() {

}