# RTOS
A custom RTOS framework for the TM4C MCU which includes custom implementation of malloc and free, priority-based scheduling, mutexes and semaphores and thread management

## Features 
- **Priority-Based Scheduling:** Lets user toggle priority scheduling for the threads, where level 0 is the highest priority and level 15 is the lowest.
- **Custom Memory Management:** Custom implementation of malloc and free to prevent non-deterministic behaviors.
- **Mutex and Semaphores:** Resource management for threads avoid deadlocks and control access to shared resources.  
- **Shell Interface:** Gives user access to manage threads - kill, restart, check pid or view memory and CPU usage.

  
## Shell Interface
<p align = center>
<img src = "Documentation/meminfo.png" width="500" >
 <p align="center"> <b> Meminfo displays thread priority, thread name, thread memory address and memory size </b> </p>
</p>

<p align = center>
<img src = "Documentation/ipcs.png" width="500" >
</p>

<p align = center>
<img src = "Documentation/ps_command.png" width="500" >
</p>
