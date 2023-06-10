#define _XOPEN_SOURCE 700
// Headers
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "src/queue.h"
#include "src/handler.h"
#include "src/reader.h"
#include "src/analyzer.h"
#include "src/printer.h"
#include "src/watchdog.h"
#include "src/logger.h"

// Defines
#define NUM_CORES 4
#define MAX_BUFFER 256
#define QUEUE_SIZE 10

volatile sig_atomic_t flagSigTerm = 0;
volatile sig_atomic_t watchdogFlag = 0;                // Flag which detects when watchdog detected an issue
Thread_Data_t threads[NUM_CORES];                      // Array of Thread_Data_t structures to store data for each thread.
Queue *cpuUsageQueue;                                  // Queue for CPU usage data
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER; // Mutex lock for synchronizing access to the queue
pthread_t printer_thread, watchdog_thread;             // Initialize printer and watchdog as global threads
time_t startTime;                                      // Timer to store the starting time

void *readerThread(void *arg);
void *analyzerThread(void *arg);
void *watchdogThread();

int main()
{
    Logger_Data_t loggerData;
    struct sigaction action;
    startTime = time(NULL);
    memset(&action, 0, sizeof(struct sigaction));
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdisabled-macro-expansion"
/*
The sa_handler in __sigaction_handler.sa_handler refers to a member of the __sigaction_handler
union, not the sa_handler macro, so I decided to disable this warning for the clang compilier.
*/
#endif
    action.sa_handler = sigHandler;
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif
    sigaction(SIGTERM, &action, NULL);
    printf("Process ID: %d\n", getpid());
    cpuUsageQueue = initQueue(QUEUE_SIZE);       // Initialize the queue
    loggerData.log_file = fopen("log.txt", "w"); // Open the log file for writing
    if (loggerData.log_file == NULL)
    {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(&loggerData.log_file_lock, NULL); // Initialize the log file lock

    if (loggerData.log_file == NULL)
    {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    // Reader and analyzer thread initialization
    for (int i = 0; i < NUM_CORES; i++)
    {
        threads[i].thread_index = i;
        threads[i].is_alive_analyzer = false;
        threads[i].is_alive_reader = false;
        pthread_create(&threads[i].reader_thread_id, NULL, readerThread, &threads[i]);
        pthread_create(&threads[i].analyzer_thread_id, NULL, analyzerThread, &threads[i]);
    }

    // Printer thread initialization
    pthread_create(&printer_thread, NULL, printerThread, NULL);

    // Watchdog thread initialization
    pthread_create(&watchdog_thread, NULL, watchdogThread, threads);

    // Logger thread initialization
    pthread_create(&loggerData.logger_thread_id, NULL, loggerThread, &loggerData);

    while (!flagSigTerm)
    {
        for (int i = 0; i < NUM_CORES; i++)
        {
            pthread_join(threads[i].reader_thread_id, NULL);
            pthread_join(threads[i].analyzer_thread_id, NULL);
        }

        pthread_join(printer_thread, NULL);
        pthread_join(watchdog_thread, NULL);
        pthread_join(loggerData.logger_thread_id, NULL);
    }

    // Cleanup
    // Close the log file
    pthread_mutex_lock(&loggerData.log_file_lock);
    if (fclose(loggerData.log_file) != 0)
    {
        perror("Failed to close log file");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&loggerData.log_file_lock);
    // Free up queue memory
    freeQueue(cpuUsageQueue);
    printf("Cleaning successful\n");

    return 0;
}
