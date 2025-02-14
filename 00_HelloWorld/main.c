/* Include necessary headers */
#include <signal.h>   /* For signal handling functions (e.g., sigaction) */
#include <time.h>     /* For timer functions (e.g., timer_create, timer_settime) */
#include <stdio.h>    /* For standard I/O functions (e.g., printf) */
#include <stdlib.h>   /* For standard library functions (e.g., exit) */
#include <stdint.h>   /* For fixed-width integer types (not used in this example) */
#include <unistd.h>   /* For POSIX API functions (e.g., pause) */
#include <pthread.h>  /* For thread-related functions (e.g., pthread_self) */

/* Signal handler function */
void signal_handler(int sig) {
    /* This function is called when the process receives the specified signal */
    printf("Handler is called\n");  /* Print a message when the handler is invoked */
}

int main() {
    /* Step 1: Set up the signal handler */
    struct sigaction act;  /* Structure to define the signal handling behavior */
    act.sa_handler = signal_handler;  /* Set the handler function */
    act.sa_flags = 0;  /* No special flags */
    sigemptyset(&act.sa_mask);  /* Initialize the signal mask (no signals are blocked during handler execution) */

    /* Step 2: Create a timer */
    timer_t timerid;  /* Variable to store the timer ID */
    struct sigevent sevp;  /* Structure to define how the timer notifies the process */

    /* Configure the sigevent structure */
    sevp.sigev_signo = SIGALRM;  /* Specify the signal to be sent (SIGALRM) */
    sevp._sigev_un._tid = pthread_self();  /* Set the thread ID (not typically used for timers) */
    sevp.sigev_notify = SIGEV_SIGNAL;  /* Notify using a signal */

    /* Step 3: Set up the timer interval and initial expiration time */
    struct itimerspec value;
    value.it_interval.tv_sec = 1;  /* Interval for periodic timer (1 second) */
    value.it_interval.tv_nsec = 0; /* No nanoseconds */
    value.it_value.tv_sec = 0;     /* Initial expiration time (0 seconds) */
    value.it_value.tv_nsec = 1;    /* Initial expiration time (1 nanosecond) */

    /* Step 4: Create the timer */
    if (timer_create(CLOCK_PROCESS_CPUTIME_ID, &sevp, &timerid) == -1) {
        /* If timer creation fails, print an error and exit */
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    /* Step 5: Register the signal handler for SIGALRM */
    if (sigaction(SIGALRM, &act, NULL) == -1) {
        /* If sigaction fails, print an error and exit */
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Step 6: Start the timer */
    if (timer_settime(timerid, 0, &value, NULL) == -1) {
        /* If timer_settime fails, print an error and exit */
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    /* Step 7: Keep the program running */
    while (1) {
        /* Infinite loop to keep the program alive and handle signals */
        /* Without this loop, the program would terminate immediately */
    }

    return 0;  /* This line is never reached due to the infinite loop */
}