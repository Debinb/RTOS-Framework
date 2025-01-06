# RTOS
A custom RTOS framework for the TM4C MCU which includes custom implementation of malloc and free, priority-based scheduling, mutexes and semaphores and thread management.

## Features 
- **Priority-Based Scheduling:** Lets user toggle priority scheduling for the threads, where level 0 is the highest priority and level 15 is the lowest. By default, there are 10 threads running with Idle being the lowest priority.
- **Custom Memory Management:** Custom implementation of malloc and free to prevent non-deterministic behaviors.
- **Mutex and Semaphores:** Resource management for threads avoid deadlocks and control access to shared resources.  
- **Shell Interface:** Gives user access to manage threads - kill, restart, check pid or view memory and CPU usage.

  
## Shell Interface
<body>
<p align = center>
<img src = "Documentation/meminfo.png" width="500" >
 <p align="center"> Meminfo displays thread priority, name, memory address and memory size </p>
</p>
</body>

<body>
<p align = center>
<img src = "Documentation/ipcs.png" width="500" >
  <p align="center"> Ipcs shows the status of the mutex and semaphores </p>
</p>
</body>

<body>
<p align = center>
<img src = "Documentation/ps_command.png" width="500" >
  <p align="center"> Ps displays the CPU usage </p>
</p>
</body>
