// Kernel functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "mm.h"
#include "kernel.h"
#include "sp.h"
#include "getInput.h"
#include "uart0.h"

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

// semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, all memory freed
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by semaphore
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

// task
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks

// control
bool priorityScheduler = true;    // priority (true) or round-robin (false)
bool priorityInheritance = false; // priority inheritance for mutexes
bool preemption = false;          // preemption (true) or cooperative (false)

// tcb
#define NUM_PRIORITIES   16
struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread (add of task fn)
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    uint8_t priority;              // 0=highest
    //uint8_t currentPriority;       // 0=highest (needed for pi)
    uint32_t* BaseAddress;
    uint32_t ticks;                // ticks until sleep complete
    uint64_t srd;                  // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    uint8_t mutex;                 // index of the mutex in use or blocking the thread
    uint8_t semaphore;             // index of the semaphore that is blocking the thread
    uint32_t* Allocation;
    uint32_t ThreadSize;
} tcb[MAX_TASKS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);
    if (ok)
    {
        mutexes[mutex].lock = false;
        mutexes[mutex].lockedBy = 0;
    }
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// REQUIRED: initialize systick for 1ms system timer
void initRtos(void)
{
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }

    NVIC_ST_RELOAD_R |= 39999;      //40Mhz system clock @ 1 Khz = 40,000 - 1
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;  //Enables Systick Timer and interrupt generation
}

// REQUIRED: Implement prioritization to NUM_PRIORITIES
// Use an array to keep track of last ran task in a priority level and
// go through each priority level and go through the tasks to see which
// one is at that priority index and is READY.
uint8_t rtosScheduler(void)
{
    bool ok;
    static uint8_t task = 0xFF;
    ok = false;

    if(priorityScheduler == true)
    {
        static uint8_t LastRanTask[NUM_PRIORITIES] = {0xFF};   //Array to track the last task at each prio level
        uint8_t HighestPriority = 16;       //Start checking from lowest priority
        uint8_t i = 0;

        //Note to self: Use MAX Tasks for the for loop since there is only 12 indices in the tcb. Stop thinking of the prio levels
        for (i = 0; i < MAX_TASKS; i++)
        {
            //Go through each tcb index and look for READY threads at the updated priority level
            if (tcb[i].state == STATE_READY && tcb[i].priority < HighestPriority)
            {
                HighestPriority = tcb[i].priority;  //update the priority level found and keep going lower if tasks at higher prio are found
                task = i;                           //Sets the task index for the READY task
            }
        }

        //Round robin at a priority level when there are multiple tasks
        if (task != 0xFF)
        {
            //start from the last scheduled task in that priority level + 1
            uint8_t LastScheduledTask = (LastRanTask[HighestPriority] + 1) % taskCount;
            uint8_t j = 0;
            for (j = 0; j < taskCount; j++)
            {
                uint8_t taskIndex = (LastScheduledTask + j) % taskCount;
                if (tcb[taskIndex].state == STATE_READY && tcb[taskIndex].priority == HighestPriority)
                {
                    task = taskIndex;
                    LastRanTask[HighestPriority] = taskIndex;   //update the last scheduled index for this priority
                    break;
                }
            }
        }
        return task;
    }
    else                //run as round-robin
    {
        while (!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY);
        }
        return task;
    }
   // return 0xFF;
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, set srd bits, setting PSP, ASP bit, call fn with fn add in R0
// fn set TMPL bit, and PC <= fn
void startRtos(void)
{
    setPSP((uint32_t*)0x20008000);
    setASP();
    setTMPL();
    __asm("     SVC #0");
}

// REQUIRED:
// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// set the srd bits based on the memory allocation
// initialize the created stack to make it appear the thread has run before
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;

            uint32_t* InitialAlloc = mallocFromHeap(stackBytes);
            uint32_t* p = (uint32_t*)((uint32_t)InitialAlloc + RoundUp(stackBytes) - 4);


            while (tcb[i].state != STATE_INVALID) {i++;}
            tcb[i].state = STATE_READY;
            tcb[i].pid = fn;
            tcb[i].sp = (void*)p;  //sets current sp to top of the stack
            tcb[i].spInit = tcb[i].sp;       //initialize sp
            tcb[i].priority = priority;

            StringCopy((char*)name, tcb[i].name);
            uint64_t srdbits = createNoSramAccessMask();
            addSramAccessWindow(&srdbits, InitialAlloc, stackBytes);   //check srd bits
            tcb[i].srd = srdbits;

            tcb[i].BaseAddress = InitialAlloc;
            tcb[i].ThreadSize = stackBytes;

            //Hardware push pop
            *(--p) = 0x01000000;     // xPSR (valid bit)
            *(--p) = (uint32_t)tcb[i].pid;     // PC
            *(--p) = 0x00000001;     // LR trash value doesn't matter
            *(--p) = 0x12121212;     // R12
            *(--p) = 0x03030303;     // R3
            *(--p) = 0x02020202;     // R2
            *(--p) = 0x01010101;     // R1
            *(--p) = 0x0000001A;     // R0
            //Software push pop
            *(--p) = 0xDEAD0004;     // R4
            *(--p) = 0xDEAD0005;     // R5
            *(--p) = 0xDEAD0006;     // R6
            *(--p) = 0xDEAD0007;     // R7
            *(--p) = 0xDEAD0008;     // R8
            *(--p) = 0xDEAD0009;     // R9
            *(--p) = 0xDEAD0010;     // R10
            *(--p) = 0xDEAD0011;     // R11
            *(--p) = (uint32_t)0xFFFFFFFD;     // EXC_RETURN Value

            tcb[i].sp = p;

            // increment task count
            taskCount++;
            ok = true;
        }
    }
    return ok;
}

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
{
    __asm("  SVC #14");
}

// REQUIRED: modify this function to stop a thread
// REQUIRED: remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
    __asm("  SVC #13");
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
    __asm("  SVC #15");
}

// REQUIRED: modify this function to yield execution back to scheduler using pendsv
void yield(void)
{
    __asm("  SVC #1");
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    __asm("  SVC #2");
}

// REQUIRED: modify this function to lock a mutex using pendsv
void lock(int8_t mutex)
{
    __asm("  SVC #3");
}

// REQUIRED: modify this function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
    __asm("  SVC #4");
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm("  SVC #5");
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm("  SVC #6");
}

void* MallocWrapper(uint32_t SizeInBytes)
{
    __asm("  SVC #7");
}

void* PIDgetter(void)
{
    return tcb[taskCurrent].pid;
}

void KillThread(void* arg)
{
    uint32_t TaskPID = (uint32_t)arg;                       // Get the PID if Kill() is called or from function call
    char *TaskName = (char*)arg;                  // Get the Name if Pkill() is called

    uint8_t task = 0;
    for(task = 0; task < MAX_TASKS; task++)             // Go through the TCB
    {
        if((TaskPID == (uint32_t)tcb[task].pid) || ((cmpStr(tcb[task].name, TaskName)) == 0))
        {
            if(tcb[task].state != STATE_STOPPED)
            {
                freeToHeap(tcb[task].BaseAddress);          // Use the thread's base address from the tcb to free it
                freeToHeap(tcb[task].Allocation);           // For function allocations

                if(tcb[task].state == STATE_BLOCKED_MUTEX)                          // Check if the task is in a Blocked_by_Mutex state and remove it
                {
                    uint8_t i,j = 0;
                    for (i = 0; i < mutexes[tcb[task].mutex].queueSize; i++)        // Go through the process queue and check for the task
                    {
                        if (mutexes[tcb[task].mutex].processQueue[i] == task)       // If task found, just update the queue
                        {
                            break;
                        }
                    }

                    for (j = i; j < mutexes[tcb[task].mutex].queueSize - 1; j++)    // Shift remaining tasks in the queue
                    {
                        mutexes[tcb[task].mutex].processQueue[j] = mutexes[tcb[task].mutex].processQueue[j + 1];
                    }
                    mutexes[tcb[task].mutex].queueSize--;
                }

                else if(tcb[task].state == STATE_BLOCKED_SEMAPHORE)                 // Or check if the task is in a Blocked_by_semaphore state
                {
                    semaphores[tcb[task].semaphore].count++;

                    if(semaphores[tcb[task].semaphore].queueSize > 0)               // If there are waiting tasks
                    {
                        uint8_t nextProcess = semaphores[tcb[task].semaphore].processQueue[0];
                        tcb[nextProcess].state = STATE_READY;

                        uint8_t i = 0;
                        for(i = 0; i < semaphores[tcb[task].semaphore].queueSize - 1; i++)
                        {
                            semaphores[tcb[task].semaphore].processQueue[i] = semaphores[tcb[task].semaphore].processQueue[i + 1];
                        }

                        semaphores[tcb[task].semaphore].queueSize--;
                        semaphores[tcb[task].semaphore].count--;
                    }
                }

                if(mutexes[tcb[task].mutex].lockedBy == task)       // Also check if the task is locking a resource
                {
                    mutexes[tcb[task].mutex].lock = false;          // Make the lock as false
                    //Implement unlock logic
                    if(mutexes[tcb[task].mutex].queueSize > 0)
                    {
                        uint8_t nextProcess = mutexes[tcb[task].mutex].processQueue[0];   //gets the next process in queue
                        tcb[nextProcess].state = STATE_READY;                   //set that process as ready

                        uint16_t i = 0;
                        for(i = 0; i < mutexes[tcb[task].mutex].queueSize - 1; i++)
                        {
                            mutexes[tcb[task].mutex].processQueue[i] = mutexes[tcb[task].mutex].processQueue[i + 1];
                        }
                        mutexes[tcb[task].mutex].queueSize--;
                        mutexes[tcb[task].mutex].lockedBy = nextProcess;
                    }
                }
                putsUart0(tcb[task].name);
                putsUart0(" killed. \n");
                tcb[task].srd = 0xFFFFFFFFFF;               // Change the SRD bits to 1's so that the process cannot R/W
                tcb[task].state = STATE_STOPPED;            // Set state to STOPPED
                break;
            }
            else if (tcb[task].state == STATE_STOPPED)
            {
                putsUart0("Process has already been killed. \n");
            }
        }
    }
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr(void)
{
    uint8_t i = 0;
    for(i = 0; i < taskCount; i++) //iterate through tcb
    {
        if(tcb[i].state == STATE_DELAYED)
        {
            tcb[i].ticks--;
            if(tcb[i].ticks == 0)  //check if ticks are 0, then change state to ready
            {
                tcb[i].state = STATE_READY;
            }
        }
    }

    if(preemption == true)
    {
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
    }
}


// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
// The PendSV exception runs at the lowest priority level for context switching when needed.
// This is needed when the OS code in the SysTick handler needs to carry out a context switch but has detected that the processor is servicing another interrupt.
__attribute__((naked))void pendSvIsr(void)
{
    __asm("  MOV R12, LR ");
    if((NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_DERR) || (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_IERR))
    {
        NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_DERR & NVIC_FAULT_STAT_IERR;
        putsUart0("Called from MPU.\n");
    }

    pushREGS();                             //save registers
    tcb[taskCurrent].sp = (void*)getPSP();  //save psp
    taskCurrent = rtosScheduler();          //call scheduler

    setPSP(tcb[taskCurrent].sp);            //restore PSP
    applySramAccessMask(tcb[taskCurrent].srd); //restore SRD
    popREGS();                              //restore regs
}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
// SVC handles cases even when in unpriveleged mode.
void svCallIsr(void)
{
    uint32_t SvcNum = getSVCnum();
    uint32_t* psp = getPSP();

    switch(SvcNum)
    {
    //Launch Task
    case 0:
    {
        taskCurrent = rtosScheduler();
        applySramAccessMask(tcb[taskCurrent].srd); //restore SRD mask
        setPSP(tcb[taskCurrent].sp);               //restore PSP
        popREGS();                  //restore registers - Popping the registers
        break;
    }

    //Yield
    case 1:
    {
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;   //Calls PendSV
        break;
    }

    //Sleep
    case 2:
    {
        uint32_t Taskticks = *psp;                      //grabs the value of the ticks from PSP address
        tcb[taskCurrent].state = STATE_DELAYED;     //changes the state of the current task to DELAYED
        tcb[taskCurrent].ticks = Taskticks;         //Stores the value of ticks to the TCB
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;   //Calls the PendSV for context switching
        break;
    }

    //Lock - check if same task is locked or not
    case 3:
    {
        int8_t mutex = *psp;
        if(mutexes[mutex].lock == false)            //if mutex index is not locked
        {
            mutexes[mutex].lock = true;             //lock it
            mutexes[mutex].lockedBy = taskCurrent;  //indicate the mutex index is locked by the current task
            tcb[mutex].mutex = mutex;               //store the mutex index in the TCB
        }
        else if(mutexes[mutex].lockedBy != taskCurrent)
        {
            tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;
            mutexes[mutex].processQueue[mutexes[mutex].queueSize] = taskCurrent;  //marking that a thread is blocked by adding it to the process queue, which requires the size of the queue.
            mutexes[mutex].queueSize++;
            tcb[mutex].mutex = mutex;               //store the mutex index in the TCB
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
        }
        else
        {
            //Kill that thread
        }
        break;
    }

    //Unlock
    case 4:
    {
        int8_t mutex = *psp;
        if(mutexes[mutex].lockedBy == taskCurrent)              //if the current task is locked by mutex
        {
            mutexes[mutex].lock = false;                        //then unlock it

            if(mutexes[mutex].queueSize > 0)                        //if there is a process waiting
            {
                uint8_t nextProcess = mutexes[mutex].processQueue[0];   //gets the next process in queue
                tcb[nextProcess].state = STATE_READY;                   //set that process as ready
                uint16_t i = 0;
                for(i = 1; i < mutexes[mutex].queueSize; i++)
                {
                    mutexes[mutex].processQueue[i - 1] = mutexes[mutex].processQueue[i];
                }
                mutexes[mutex].queueSize--;

                // Next task is now locking the mutex
                mutexes[mutex].lock = true;
                mutexes[mutex].lockedBy = nextProcess;      //indicates mutex index is held by next Process
            }
        }
        break;
    }

    //wait
    case 5:
    {
        int8_t sema = *psp;
        if(semaphores[sema].count > 0)
        {
            semaphores[sema].count--;           //decrement count
        }
        else
        {
            semaphores[sema].processQueue[semaphores[sema].queueSize] = taskCurrent;    //put the task in queue
            semaphores[sema].queueSize++;                                               //increment queue count
            tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;                           //change state to BLOCKED
            tcb[taskCurrent].semaphore = sema;                                          //put semaphore index in the tcb
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;                                   //call PendSV
        }
        break;
    }

    //post
    case 6:
    {
        int8_t sema = *psp;
        semaphores[sema].count++;
        if(semaphores[sema].queueSize > 0)      //if some task in the queue
        {
            uint8_t nextProcess = semaphores[sema].processQueue[0];     //make next task ready by getting next process in queue
            tcb[nextProcess].state = STATE_READY;
            uint16_t i = 0;
            for(i = 1; i < semaphores[sema].queueSize; i++)
            {
                semaphores[sema].processQueue[i-1] = semaphores[sema].processQueue[i];  //move the user in queue
            }
            semaphores[sema].queueSize--;              //decrement queue count
            semaphores[sema].count--;
            tcb[sema].semaphore = sema;               //store the semaphore index in the TCB
        }
        break;
    }

    case 7: //Malloc for LengthyFn
    {
        uint16_t Size = *psp;                                    //gets 5000 from R0
        void* Address = mallocFromHeap(Size);           //returns 0x20006C00. Ends at 0x20007C00
        tcb[taskCurrent].Allocation = Address;
        addSramAccessWindow(&tcb[taskCurrent].srd, Address, Size);
        applySramAccessMask(tcb[taskCurrent].srd);                   //srdBits = 0x07.FFFF.FFFF
        *psp = (uint32_t)Address;                       //Stores 0x20006C00 into r0
        break;
    }

    //===========================SHELL INTERFACE================================//
    case 8:                                 //SCHEDULER
    {
        priorityScheduler = *getPSP();
        break;
    }

    case 9:                                //PREEMPTION
    {
        preemption = *getPSP();
        break;
    }

    case 10:                              //PRIORITY INHERITANCE
    {
        priorityInheritance = *getPSP();
        break;
    }

    case 11:                            //PIDOF
    {
        char *ThreadName = (char*)*getPSP();    //gets the thread name passed
        uint8_t i = 0;
        bool found = 0;
        uint16_t pidValue = 0;
        char str[15];

        for(i = 0; i < MAX_TASKS; i++)
        {
            if((cmpStr(tcb[i].name, ThreadName)) == 0)
            {
                found = 1;
                //*getPSP() = (uint32_t)tcb[i].pid;
                pidValue = (uint32_t)tcb[i].pid;
                break;
            }
        }

        if(!found)
        {
            *getPSP() = 0;
            putsUart0("PID not found.\n");
        }
        else if(found == 1)
        {
            IntToStr(pidValue, str);
            putsUart0("PID: ");
            putsUart0(str);
            putcUart0('\n');
        }
        break;
    }

    case 12:                            //REBOOT
    {
        NVIC_APINT_R = (NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ);      //System Reset Request
        break;
    }
    //====================================================================//

    //Stop thread
    case 13:
    {
        uint32_t TaskPID = *getPSP();
        KillThread((void*)TaskPID);
        break;
    }

    //RESTART THREAD
    case 14:
    {
        uint32_t TaskPID = *getPSP();                       // Get the PID if Kill() is called or from function call
        char *TaskName = (char *)*getPSP();                  // Get the task Name passed into restartthread()

        uint8_t task = 0;
        for(task = 0; task < MAX_TASKS; task++)             // Go through the TCB
        {
            if((TaskPID == (uint32_t)tcb[task].pid) || ((cmpStr(tcb[task].name, TaskName)) == 0))
            {
                if(tcb[task].state == STATE_STOPPED)        //check for process states so that only stopped ones get restarted or else mem error might occur
                {
                    // allocate the stack and size to SP
                    uint32_t* NewAllocation = mallocFromHeap(tcb[task].ThreadSize);

                    uint32_t* p = (uint32_t*)((uint32_t)NewAllocation + RoundUp(tcb[task].ThreadSize) - 4);
                    tcb[task].sp = (void*)p;
                    tcb[task].spInit = tcb[task].sp;
                    tcb[task].BaseAddress = NewAllocation;
                    tcb[task].ticks = 0;
                    addSramAccessWindow(&tcb[task].srd, NewAllocation, tcb[task].ThreadSize);

                    //Hardware push pop
                    *(--p) = 0x01000000;     // xPSR (valid bit)
                    *(--p) = (uint32_t)tcb[task].pid;     // Pid to pc
                    *(--p) = 0x00000001;     // LR trash value doesn't matter
                    *(--p) = 0x12121212;     // R12
                    *(--p) = 0x03030303;     // R3
                    *(--p) = 0x02020202;     // R2
                    *(--p) = 0x01010101;     // R1
                    *(--p) = 0x0000001A;     // R0
                    //Software push pop
                    *(--p) = 0xDEAD0004;     // R4
                    *(--p) = 0xDEAD0005;     // R5
                    *(--p) = 0xDEAD0006;     // R6
                    *(--p) = 0xDEAD0007;     // R7
                    *(--p) = 0xDEAD0008;     // R8
                    *(--p) = 0xDEAD0009;     // R9
                    *(--p) = 0xDEAD0010;     // R10
                    *(--p) = 0xDEAD0011;     // R11
                    *(--p) = (uint32_t)0xFFFFFFFD;     // EXC_RETURN Value

                    tcb[task].sp = p;

                    tcb[task].state = STATE_READY;  // STATE to ready
                    putsUart0(tcb[task].name);
                    putsUart0(" restarted. \n");
                    break;
                }
                else
                {
                    putsUart0("Cannot restart. Process already running.\n");
                    break;
                }
            }
        }
        break;
    }

    }
}

