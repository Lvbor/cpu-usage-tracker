#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include "reader.h"

#define WATCHDOG_TIMEOUT 2
#define NUM_CORES 4

extern volatile sig_atomic_t flagSigTerm;
extern volatile sig_atomic_t watchdogFlag;
extern Thread_Data_t threads[NUM_CORES];                      // Array of Thread_Data_t structures to store data for each thread.                

// Watchdog thread
void *watchdogThread()
{

    while (!flagSigTerm)
    {
        sleep(WATCHDOG_TIMEOUT); // Sleep for the WATCHDOG_TIMEOUT duration
        
        for (int i = 0; i < NUM_CORES; i++)
        {
            if (!threads[i].is_alive_analyzer || !threads[i].is_alive_reader)
            {
                // If a thread did not report in time, print an error message
                fprintf(stderr, "Thread %d did not report in time. Exiting...\n", i);
                
                // Set the watchdogFlag to 1 to indicate an error
                watchdogFlag = 1;
                
                // Raise the SIGTERM signal to terminate the program
                raise(SIGTERM);
            }
            else
            {
                // Reset the alive status of both analyzer and reader threads
                threads[i].is_alive_analyzer = false;
                threads[i].is_alive_reader = false;
            }
        }
    }
    return NULL;
}