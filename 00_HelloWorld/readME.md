# **Hello World: Timer Example**

This example demonstrates how to use **POSIX timers** and **signal handling** in Linux. A timer is set to expire every **1 second**, and a signal handler prints `"Handler is called"` each time the timer triggers.

## **How to Run**
1. Save the code in a file named `main.c`.
2. Compile and run:
   ```bash
    gcc main.c -lrt -o main.out
    ./main.out
   ```
3. The program will print `"Handler is called"` every second. Press `Ctrl+C` to stop.