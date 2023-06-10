#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "handler.h"

extern volatile sig_atomic_t flagSigTerm;                 // Flag which detects SIGTERM signal

// Signal handler function for SIGTERM signal
void sigHandler(int signal)
{
    if (signal == SIGTERM)
    {
        printf("Received SIGTERM signal. Closing gracefully...\n");
        flagSigTerm = 1;
        sleep(2); // Wait 2 seconds to shutdown gracefully
    }
}