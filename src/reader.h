#ifndef READER_H
#define READER_H

#include <pthread.h>
#include <stdbool.h>

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
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpragma-pack"
#pragma pack(1) // Disable padding
#endif
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
#ifdef __clang__
#pragma pack() // Reset to default alignment rule
#pragma clang diagnostic pop
#endif

#endif // READER_H
