#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include "reader.h"
#include "logger.h"

#define NUM_CORES 4

extern volatile sig_atomic_t flagSigTerm;                 // Flag which detects SIGTERM signal
extern volatile sig_atomic_t watchdogFlag;                // Flag which detects when watchdog detected an issue
extern time_t startTime;                               // Timer to store the starting time
extern pthread_t printer_thread, watchdog_thread;      // Initialize printer and watchdog as global threads
extern Thread_Data_t threads[NUM_CORES];                      // Array of Thread_Data_t structures to store data for each thread.

void *loggerThread(void *arg)
{
    Logger_Data_t *loggerData = (Logger_Data_t *)arg;
    FILE *file = NULL;
    bool initflag = true;

    while (!flagSigTerm)
    {
        // Get the current time and calculate the program uptime
        time_t currentTime = time(NULL);
        double uptime = difftime(currentTime, startTime);
        // Open the log file
        pthread_mutex_lock(&loggerData->log_file_lock); // Lock
        file = loggerData->log_file;

        if (file != NULL)
        {
            // Print information about thread initializations
            while (initflag == true)
            {
                fprintf(file, "Thread Initialization Information:\n\n");
                for (int i = 0; i < NUM_CORES; i++)
                {
                    fprintf(file, "Thread %d:\n", i);
                    fprintf(file, "Reader Thread ID: %lu\n", threads[i].reader_thread_id);
                    fprintf(file, "Analyzer Thread ID: %lu\n", threads[i].analyzer_thread_id);
                }
                fprintf(file, "\nOthers:");
                fprintf(file, "\nPrinter Thread ID: %lu\n", printer_thread);
                fprintf(file, "Watchdog Thread ID: %lu\n", watchdog_thread);
                initflag = false;
                fprintf(file, "\nInitialization complete!\n");
            }
            if (threads->is_alive_analyzer == true && threads->is_alive_reader == true)
            {
                fprintf(file, "\nProgram uptime: %.2fs\n", uptime);
                fprintf(file, "Current data:\nidle: %lu irq: %lu nice: %lu softirq: %lu steal: %lu system: %lu user: %lu", 
                threads->curr_data.idle, threads->curr_data.irq,threads->curr_data.nice, threads->curr_data.softirq, 
                threads->curr_data.steal, threads->curr_data.system, threads->curr_data.user);
                fprintf(file, "\nAll threads are currently alive!\n");
            }
        }

        pthread_mutex_unlock(&loggerData->log_file_lock); // Release the lock

        // Sleep for 1s
        sleep(1);
    }
    if (watchdogFlag == 1)
    {
        fprintf(file, "\nWatchdog: One or more threads did not report in time.\n");
    }
    if (flagSigTerm == 1)
    {
        fprintf(file, "\nSigterm signal raised! Exitting gracefully...\n");
    }
    return NULL;
}
