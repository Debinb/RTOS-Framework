# RTOS
A custom RTOS framework for the TM4C MCU which includes custom implementation of malloc and free, priority-based scheduling, mutexes and semaphores and thread management.

## Features 
- **Priority-Based Scheduling:** Lets user toggle priority scheduling for the threads, where level 0 is the highest priority and level 15 is the lowest. By default, there are 10 threads running with Idle being the lowest priority.
- **Custom Memory Management:** Custom implementation of malloc and free to prevent non-deterministic behaviors.
- **Mutex and Semaphores:** Resource management for threads avoid deadlocks and control access to shared resources.  
- **Shell Interface:** Gives user access to manage threads - kill, restart, check pid or view memory and CPU usage.

  
## Shell Interface
- `reboot`: Reboots the TM4C MCU. 
- `kill pid`: Kill a thread using its PID.
- `pkill threadname`: Kill a thread using its thread name.
- `preempt on|off`: Toggle preemption using ON or OFF
- `sched rr|prio`: Toggle between Round-robin scheduling (_sched rr_) or priority scheduling (_sched prio_). 
- `pidof x`: Gets the pid of a thread by typing the thread name.
- `threadname`: Restarts the thread if it is stopped.
- `meminfo`: Displays thread priority, name, memory address and memory size.
<p align = center> <img src = "Documentation/meminfo.png" width="300" > </p>
- `ipcs`: Displays the status of the mutexes and semaphores.
<p align = center> <img src = "Documentation/ipcs.png" width="300" > </p>
- `ps`: Displays the thread PID, CPU usage and its state.
<p align = center> <img src = "Documentation/ps_command.png" width="500" ></p>

