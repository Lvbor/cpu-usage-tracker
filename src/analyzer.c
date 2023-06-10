#include <signal.h>
#include <unistd.h>
#include "reader.h"
#include "analyzer.h"
#include "queue.h"


extern volatile sig_atomic_t flagSigTerm;
extern Queue *cpuUsageQueue;       // Queue for CPU usage data
extern pthread_mutex_t queueLock; // Mutex lock for synchronizing access to the queue

void *analyzerThread(void *arg)
{
    Thread_Data_t *threadData = (Thread_Data_t *)arg;

    threadData->prev_data = threadData->curr_data; 

    while (!flagSigTerm)
    {
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
        if (total_diff != 0)
        {
            cpu_usage = (float)(total_diff - idle_diff) * 100.0f / (float)total_diff;
        }
        else
        {
            cpu_usage = 0.0f;
        }

        // Push the CPU usage to the queue
        pthread_mutex_lock(&queueLock);
        if (!isFull(cpuUsageQueue))
        {
            enqueue(cpuUsageQueue, cpu_usage);
        }
        pthread_mutex_unlock(&queueLock);

        // Replace previous data with current data
        threadData->prev_data = threadData->curr_data;

        // Sleep for 1s
        sleep(1);

        // Set the thread as alive
        threadData->is_alive_analyzer = true;
    }

    return NULL;
}