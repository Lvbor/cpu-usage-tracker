#ifndef LOGGER_H
#define LOGGER_H
#include <pthread.h>
#include <stdio.h>

// Logger thread data
typedef struct
{
    pthread_t logger_thread_id;    
    FILE *log_file;                
    pthread_mutex_t log_file_lock; // Mutex lock for synchronizing access to the log file
} Logger_Data_t;

void *loggerThread(void *arg);

#endif  // LOGGER_H