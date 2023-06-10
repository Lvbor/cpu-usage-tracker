#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "queue.h"

#define NUM_CORES 4

extern Queue *cpuUsageQueue;
extern volatile sig_atomic_t flagSigTerm;
extern pthread_mutex_t queueLock; // Mutex lock for synchronizing access to the queue

void *printerThread()
{

    while (!flagSigTerm)
    {
        float total_cpu_usage = 0.0f;
        float average_cpu_usage = 0.0f;

        // Fetch the CPU usage from the queue
        pthread_mutex_lock(&queueLock);
        while (!isEmpty(cpuUsageQueue))
        {
            total_cpu_usage += dequeue(cpuUsageQueue);
        }
        pthread_mutex_unlock(&queueLock);

        // Calculate the average CPU usage
        if (NUM_CORES != 0)
        {
            average_cpu_usage = total_cpu_usage / NUM_CORES;
        }
        else
        {
            average_cpu_usage = 0.0f;
        }

        printf("Average CPU usage: %.2f%%\n", (double)average_cpu_usage);

        // Sleep for 1s
        sleep(1);
    }

    return NULL;
}