# POSIX Interval Timers

POSIX Interval timers could be created and handled using the following System calls

1. **timer_create()**: Create a timer.
2. **timer_settime()**: Arm (start) or disarm (stop) a timer.
3. **timer_gettime()**: Fetch the time remaining until the next expiration of a timer, along with the interval setting of the timer.
4. **timer_getoverrun()**: Return the overrun count for the last timer expiration.
5. **timer_delete()**: Disarm and delete a timer.

They are per-process timers, which means that the are not inherited by fork and deleted when exec is executed.
On Linux, programs using the POSIX timer API must be compiled with the –lrt
option, in order to link against the librt (realtime) library.

## int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);
Used to create a timer for a thread,please note that the main program is a thread called main thread even there is no child thread executing from it :).

Returns 0 on success, or –1 on error.

### Parameters:
#### 1. **clockid**
Specifies the clock that the new timer uses to measure time with the following values (From the man page of timer_create):

1. **CLOCK_REALTIME**
A settable system-wide real-time clock.

2. **CLOCK_MONOTONIC**
A nonsettable monotonically increasing clock that measures time from some unspecified point in the past that does not change after sys‐
tem startup ,ie from the system booting.

3. **CLOCK_PROCESS_CPUTIME_ID (since Linux 2.6.12)**
A clock that measures (user and system) CPU time consumed by (all of the threads in) the calling process.

4. **CLOCK_THREAD_CPUTIME_ID (since Linux 2.6.12)**
A clock that measures (user and system) CPU time consumed by the calling thread.

5. **CLOCK_BOOTTIME (Since Linux 2.6.39)**
Like  CLOCK_MONOTONIC,  this is a monotonically increasing clock.  However, whereas the CLOCK_MONOTONIC clock does not measure the time
while a system is suspended, the CLOCK_BOOTTIME clock does include the time during which the system is suspended.  This is  useful  for
applications that need to be suspend-aware.  CLOCK_REALTIME is not suitable for such applications, since that clock is affected by dis‐
continuous changes to the system clock.

5. **CLOCK_REALTIME_ALARM (since Linux 3.0)**
This clock is like CLOCK_REALTIME, but will wake the system if it is suspended.  The caller must have the CAP_WAKE_ALARM capability  in
order to set a timer against this clock.

6. **CLOCK_BOOTTIME_ALARM (since Linux 3.0)**
This  clock is like CLOCK_BOOTTIME, but will wake the system if it is suspended.  The caller must have the CAP_WAKE_ALARM capability in
order to set a timer against this clock.

7. **CLOCK_TAI (since Linux 3.10)**
A system-wide clock derived from wall-clock time but ignoring leap seconds.



What we will use is **CLOCK_PROCESS_CPUTIME_ID**, because it counts the time for the whole process including the threads running inside it, which meets our requirements in our project.

--- 

#### 2. **timerid**
It is an output parameter specifies the unique ID of the interval timer, which is unique per process, and if the timer is deleted, then the deleted timer id could be used by another timer.

---

#### 3. **evp** Struct:
It is a struct used to configure our timer action, which thread will receive the signal, and if there is any parameter will be passed to it.It includes
1. **sigev_signo**: The number of the signal send to the threads which are interested with our timer.
2. **sigev_value**: It is a struct contains parameters passed to the signal handler or the thread function.
3. **_sigev_un**: A struct contains the information related to the thread which will be noticed. It may contain the thread ID or the thread handler
4. **sigev_notify**: The Notification Method of the thread. It could be one of the following values.
    1. **SIGEV_NONE**: No signal will be send, just read the timer value using **timer_gettime()**
    2. **SIGEV_SIGNAL**: When the timer expires, generate the signal specified in the sigev_signo field for the process. If sigev_signo is a realtime signal, then the sigev_value field specifies data (an integer or a pointer) to accompany the signal.This data can be retrieved via the si_value field of the siginfo_t structure that is passed to the handler for this signal or returned by a call to sigwaitinfo() or sigtimedwait()
    3. **SIGEV_THREAD**: Not available in windows.SUS v3 only.
    4. **SIGEV_THREAD_ID**: Not available in windows.In Linux only.

**Note**

The **evp** argument may be specified as NULL, which is equivalent to specifying:
1. sigev_notify as SIGEV_SIGNAL.
2. sigev_signo as SIGALRM (this may be different on other systems,since SUSv3 merely says “a default signal number”)
3. sigev_value.sival_int as the timer ID.

---
so sigevent could be implemented as the following:

```c
union sigval {
    int sival_int;   /* Integer value for accompanying data */
    void *sival_ptr; /* Pointer value for accompanying data */
};

struct sigevent {
    int sigev_notify; /* Notification method */
    int sigev_signo; /* Timer expiration signal */
    union sigval sigev_value; /* Value accompanying signal or passed to thread function */
    union {
        pid_t _tid; /* ID of thread to be signaled */
        struct {
            void (*_function) (union sigval);/* Thread notification function */
            void *_attribute; /* Really 'pthread_attr_t *' */
        } _sigev_thread;
    } _sigev_un;
};

#define sigev_notify_function _sigev_un._sigev_thread._function
#define sigev_notify_attributes _sigev_un._sigev_thread._attribute
#define sigev_notify_thread_id _sigev_un._tid
```

### Example 
This example initialize a timer and the signal handling it. This Example won't run until we set the interval of timer.It is made just to keep up:).

```c
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

    /* Step 3: Create the timer */
    if (timer_create(CLOCK_PROCESS_CPUTIME_ID, &sevp, &timerid) == -1) {
        /* If timer creation fails, print an error and exit */
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    /* Step 4: Register the signal handler for SIGALRM */
    if (sigaction(SIGALRM, &act, NULL) == -1) {
        /* If sigaction fails, print an error and exit */
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Step 5: Keep the program running */
    while (1) {
        /* Infinite loop to keep the program alive and handle signals */
        /* Without this loop, the program would terminate immediately */
    }

    return 0;  /* This line is never reached due to the infinite loop */
}
```

---

## int timer_settime(timer_t timerid, int flags, const struct itimerspec *value, struct itimerspec *old_value);

This System call is used to define the time for the next expiration with the following arguments
1. **timerid**: The id of the timer which is the input of `timer_create` System call
2. **value**: A struct of type `itimerspec` used to specify the settings of the timer with the following members.
```c
struct itimerspec {
 struct timespec it_interval; /* Interval for periodic timer */
 struct timespec it_value; /* First expiration */
};
```
To configure a periodic timer
1. **it_interval**: This should specify the perodicity
2. **it_value**: This should specify when the first time the timer will expire **and it shouldn't be zero**

To configure a one-shot timer
1. **it_interval**: This should be zero
2. **it_value**: This should specify when the timer will expire

Both **it_interval** and **it_value** are of type `timespec` which is a struct with the following members:

```c
struct timespec {
 time_t tv_sec; /* Seconds */
 long tv_nsec; /* Nanoseconds */
};
```
3. **flags**: It specifies whether **it_value** is a relative time or absolute time.
In case of relative time, it is relative to the time of calling `timer_settime`.
In case of absolute time, it is counted from the clock’s zero point

- relative time -> flags = 0
- absolute time -> flags = TIMER_ABSTIME

If the timer value and interval are not multiples of the resolution of the corresponding clock,these values are rounded up to the next multiple of the resolution

4. **old_value**: you can call timer_settime to reconfigure the timer, so the old settings are returned at **old_value**, you can ignore it by setting it as NULL

Example

```c
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
```

run using the following cmd

```bash
gcc main.c -lrt -o main.out
```

Refere to **HelloWorld** Example

---

## int timer_gettime(timer_t timerid, struct itimerspec *curr_value);

- It retrives the time remaining until the next expiration of the timer and the timer perodicity

1. **timerid**: The Timer ID which you want to retrive the settings.

2. **curr_value**:
    1. **it_value**: The time till the next expiration, if it equals 0 then the timer is already disarmed (inactive).

    2.**it_interval**  : The perodicity of the timer, if it equals zero then the timer is one-shot timer.
## timer_delete(timer_t timerid)
disarm the armed timer given by **timerid** and if there is a signal pending fired by this timer, it will still pending until the activation of the timer.
The timer will not be deleted until the termination of the process.

## int timer_getoverrun(timer_t timerid);

returns the number of overruns of the timer given by **timerid** or -1 in case of error.