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

// Defines
#define NUM_CORES 4
#define MAX_BUFFER 256
#define WATCHDOG_TIMEOUT 2
#define QUEUE_SIZE 10

// A structure that stores data for each core
typedef struct
{
    unsigned long user;    // Time used for user tasks
    unsigned long nice;    // Time used for "nice" tasks
    unsigned long system;  // Time used for system tasks
    unsigned long idle;    // Processor idle time
    unsigned long irq;     // Time used for servicing interrupts
    unsigned long softirq; // Time used for servicing software interrupts
    unsigned long steal;   // Stolen time (virtualization)
} CPU_Data_t;

// A structure that stores data for each thread
#pragma pack(1) // Disable padding
typedef struct
{
    CPU_Data_t prev_data;
    CPU_Data_t curr_data;
    pthread_t reader_thread_id;
    pthread_t analyzer_thread_id;
    __int32_t thread_index;
    bool is_alive_reader;   // Thread status for watchdog thread
    bool is_alive_analyzer; // Thread status for watchdog thread
} Thread_Data_t;
#pragma pack(4) // Reset to previous alignment rule (enable padding)

// Logger thread data
typedef struct
{
    pthread_t logger_thread_id;    
    FILE *log_file;                
    pthread_mutex_t log_file_lock; // Mutex lock for synchronizing access to the log file
} Logger_Data_t;


typedef struct node
{
    float value;       // Value stored in the node
    struct node *next; // Pointer to the next node in the linked list
} Node;

typedef struct queue
{
    Node *head;   // Pointer to the head of the queue
    Node *tail;   // Pointer to the tail of the queue
    int count;    // Number of elements currently in the queue
    int capacity; // Maximum capacity of the queue
} Queue;

static volatile sig_atomic_t flagSigTerm = 0;                 // Flag which detects SIGTERM signal
static volatile sig_atomic_t watchdogFlag = 0;                // Flag which detects when watchdog detected an issue
static Thread_Data_t threads[NUM_CORES];                      // Array of Thread_Data_t structures to store data for each thread.
static Queue *cpuUsageQueue;                                  // Queue for CPU usage data
static pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER; // Mutex lock for synchronizing access to the queue
static pthread_t printer_thread, watchdog_thread;             // Initialize printer and watchdog as global threads
static time_t startTime; // Timer to store the starting time

// Queue initialization
static Queue *initQueue(int capacity)
{
    Queue *queue = (Queue *)malloc(sizeof(Queue)); // Allocate memory for the Queue structure
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    queue->capacity = capacity;
    return queue;
}

// Adding elements to the queue
static void enqueue(Queue *queue, float value)
{
    // Create a new node
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->value = value;
    newNode->next = NULL;
    
    // If the queue is full, return without adding the new node
    if (queue->count == queue->capacity)
    {
        return;
    }
    
    // If the queue is not empty, update the next pointer of the current tail to the new node
    if (queue->tail != NULL)
    {
        queue->tail->next = newNode;
    }
    
    // Update the tail of the queue to point to the new node
    queue->tail = newNode;
    
    // If the queue is empty, update the head to point to the new node as well
    if (queue->head == NULL)
    {
        queue->head = newNode;
    }
    
    queue->count++;
}

// Taking elements from the queue
static float dequeue(Queue *queue)
{
    // Store the head node
    Node *temp = queue->head;
    
    // Store the value of the head node
    float value = temp->value;
    
    // If the queue is empty, return -1
    if (queue->count == 0)
    {
        return -1.0;
    }
    
    // Update the head of the queue to point to the next node
    queue->head = queue->head->next;
    
    if (queue->head == NULL)
    {
        queue->tail = NULL;
    }
    
    // Free the memory allocated for the dequeued node
    free(temp);
    
    queue->count--;
    
    return value;
}

static int isFull(Queue *queue)
{
    // Check if the count of elements in the queue is equal to the queue's capacity
    return queue->count == queue->capacity;
}

static int isEmpty(Queue *queue)
{
    // Check if the count of elements in the queue is 0
    return queue->count == 0;
}

static void freeQueue(Queue *queue)
{
    Node *temp = queue->head;
    Node *nextNode;

    // Deallocate all nodes
    while (temp != NULL)
    {
        nextNode = temp->next;
        free(temp);
        temp = nextNode;
    }

    // Deallocate the queue
    free(queue);
}

// Signal handler function for SIGTERM signal
static void sigHandler(int signal)
{
    if (signal == SIGTERM)
    {
        printf("Received SIGTERM signal. Closing gracefully...\n");
        flagSigTerm = 1;
        sleep(2); // Wait 2 seconds to shutdown gracefully
    }
}

// Reader thread
static void *readerThread(void *arg)
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

static void *analyzerThread(void *arg)
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

static void *printerThread()
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

static void *watchdogThread()
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

static void *loggerThread(void *arg)
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
