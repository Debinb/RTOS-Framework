// Shell functions
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
#include <stdio.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#include "gpio.h"
#include "uart0.h"
#include "shell.h"
#include "kernel.h"
#include "sp.h"

// REQUIRED: Add header files here for your strings functions, ...
#include "getInput.h"


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void meminfo()
{
    ExtractTCB showTCB[MAX_TASKS];
    getTCBinfo(showTCB);

    putsUart0("-------------------------------------------\n");
    putsUart0("| Prio | Process Name |  Address   | Size  \n");
    putsUart0("-------------------------------------------\n");

    uint8_t i,j = 0;
    char info[15];
    for(i = 0; i < MAX_TASKS; i++)
    {
        if(showTCB[i].state != 1)
        {
            if(showTCB[i].ThreadSize == 0)  // For not displaying empty values
            {
                break;
            }


            putsUart0("|  ");
            IntToStr(showTCB[i].priority,info);
            putsUart0(info);
            for (j = StringLen(info); j < 4; j++)
            {
                putsUart0(" ");
            }
            //Process name
            putsUart0("| ");
            putsUart0(showTCB[i].name);
            for (j = StringLen(showTCB[i].name); j < 11; j++)
            {
                putsUart0(" ");
            }

            //Base Address
            putsUart0("  | ");
            IntToHex((uint32_t)showTCB[i].BaseAddr, info);
            putsUart0("0x");
            putsUart0(info);

            //Thread Size
            putsUart0(" | ");
            IntToStr(showTCB[i].ThreadSize, info);
            putsUart0(info);
            for (j = StringLen(info); j < 5; j++)
            {
                putsUart0(" ");
            }
            putsUart0("B\n");
        }
    }
}

void ps()
{
    ExtractTCB showTCB[MAX_TASKS];
    getTCBinfo(showTCB);

    putsUart0("--------------------------------------------------------------------\n");
    putsUart0("|  PID  | Process Name |  CPU%  |        State         | Blocked By  \n");
    putsUart0("--------------------------------------------------------------------\n");

    uint8_t i,j,k = 0;
    char info[15];
    for(i = 0; i < MAX_TASKS; i++)
    {
        if(showTCB[i].ThreadSize == 0)
        {
            break;
        }
        if (showTCB[i].pid == showTCB[i+1].pid) //remove extra lengthyfn
        {
            i++;
            k = 1;
        }

        //PID
        putsUart0("| ");
        IntToStr(showTCB[i].pid, info);
        putsUart0(info);
        for(j = StringLen(info); j < 6; j++)
        {
            putsUart0(" ");
        }

        //PROCESS NAME
        putsUart0("| ");
        putsUart0(showTCB[i].name);
        for(j = StringLen(showTCB[i].name); j < 10; j++)
        {
            putsUart0(" ");
        }
        putsUart0("   | ");

        //CPU TIME
        if(showTCB[i].CPU_TIME == 0)
        {
            putsUart0("0.00");
            putsUart0("% ");   //For cpu time - change later

            for(j = StringLen(info); j < 6; j++)
            {
                putsUart0(" ");
            }
            putsUart0("|");

        }
        else
        {
            IntToStr(showTCB[i].CPU_TIME, info);
            DecimalPlacer(info);
            putsUart0(info);

            putsUart0("% ");   //For cpu time - change later

            for(j = StringLen(info); j < 5; j++)
            {
                putsUart0(" ");
            }
            putsUart0("|");

        }

        //PROCESS STATE
        char* CurrentState;
        switch(showTCB[i].state)
        {
        case 1:
            CurrentState = "STOPPED";
            break;
        case 2:
            CurrentState = "READY";
            break;
        case 3:
            CurrentState = "DELAYED";
            break;
        case 4:
            CurrentState = "BLOCKED BY MUTEX";
            break;
        case 5:
            CurrentState = "BLOCKED BY SEMAPHORE";
            break;
        }
        putsUart0(CurrentState);
        for (j = StringLen(CurrentState); j < 20; j++)
        {
            putsUart0(" ");
        }
        putsUart0(" | ");

        //BLOCKED BY
        if(CurrentState == "BLOCKED BY MUTEX")
        {
            putsUart0(showTCB[showTCB[i].LockedBy + k].name);
        }
        putsUart0("\n");
    }
}

void ipcs()
{
    ExtractMutexSema Status[4];
    getMutexSemaInfo(Status);

    char info[20];
    char proc_list[10][20] = {"Idle", "LengthyFn", "Flash4Hz", "OneShot", "ReadKeys", "Debounce", "Important", "Uncoop", "Errant", "Shell"};
    char sema_list[3][12] = {"keyPressed","keyReleased","flashReq"};

    putsUart0("\n--------------------Mutex Status--------------------\n\n");
    if(Status[0].lock == 1)
    {
        putsUart0("Resource locked By:   ");
        putsUart0(proc_list[Status[0].MutexLockedBy]);

        putsUart0("\nMutex Queue Size:     ");
        IntToStr(Status[0].MutexQueueSize, info);
        putsUart0(info);

        putsUart0("\nMutex Queue:      [0] ");
        if((Status[0].MutexProcessQueue[0] == 0) && (Status[0].MutexProcessQueue[1] == 0))
        {
            putsUart0("-- \n                  [1] --\n");
        }
        else if((Status[0].MutexProcessQueue[0] > 0) && (Status[0].MutexProcessQueue[1] == 0))
        {
            putsUart0(proc_list[Status[0].MutexProcessQueue[0]]);
            putsUart0("\n                  [1] --");
        }
        else
        {
            putsUart0(proc_list[Status[0].MutexProcessQueue[0]]);
            putsUart0("\n                  [1] ");
            putsUart0(proc_list[Status[0].MutexProcessQueue[1]]);
        }
    }
    else
    {
        putsUart0("All mutexes are free.\n");
    }

    putsUart0("\n\n\n------------------Semaphore Status------------------\n\n");

    uint8_t i = 0;
    for(i = 1; i < 4; i++)
    {
        putsUart0("[");
        IntToStr(i-1, info);
        putsUart0(info);
        putsUart0("]");
        putsUart0(sema_list[i-1]);
        putsUart0(":");

        if(Status[i].SemaQueue[0] > 0)
        {
            putsUart0("\tResource Counts:  \t");
            IntToStr(Status[i].SemaCount, info);
            putsUart0(info);
            putsUart0("\n");

            putsUart0("\t\tSemaphore Queue Size:  \t");
            IntToStr(Status[i].SemaQueueSize, info);
            putsUart0(info);
            putsUart0("\n");

            putsUart0("\t\tSemaphore Queue:\t[0] ");
            if(Status[i].SemaQueue[1] == 0)
            {
                putsUart0(proc_list[Status[i].SemaQueue[0]]);
                putsUart0("\n\t\t\t\t\t[1] --");
            }
            else
            {
                putsUart0(proc_list[Status[i].MutexProcessQueue[0]]);
                putsUart0("\n\t\t\t\t\t[1] ");
                putsUart0(proc_list[Status[i].MutexProcessQueue[1]]);
            }
            putsUart0("\n\n");
        }
        else
        {
            putsUart0("\tResource Counts:  \t");
            IntToStr(Status[i].SemaCount, info);
            putsUart0(info);
            putsUart0("\n");

            putsUart0("\t\tSemaphore Queue Size:\t");
            IntToStr(Status[i].SemaQueueSize, info);
            putsUart0(info);
            putsUart0("\n");

            putsUart0("\t\tSemaphore Queue:\t[0]");
            putsUart0(" -- \n\t\t\t\t\t[1] --\n");
            putsUart0("\n");
        }
    }

}

void kill(uint32_t pid)         //StopThread()
{
    stopThread((_fn)pid);
}

void pkill(char* proc_name)     //StopThread()
{
    stopThread((_fn)proc_name);
}

void preempt(bool on)
{
    __asm("  SVC #9");
    if(on == true)
    {
        putsUart0("Preemption is ON\n");
    }
    else
    {
        putsUart0("Preemption is OFF\n");
    }
}

void sched(bool prio_on)
{
    __asm("  SVC #8");
    if(prio_on == true)
    {
        putsUart0("Scheduler set to Priority.\n");
    }
    else
    {
        putsUart0("Scheduler set to Round-Robin.\n");
    }
}

void pi(bool on)        //Priority Inheritance -- not needed
{
    __asm("  SVC #10");
    if(on == true)
    {
        putsUart0("pi ON\n");
    }
    else
    {
        putsUart0("pi OFF\n");
    }
}


void pidof(char* name)
{
    __asm("  SVC #11");
}


// REQUIRED: add processing for the shell commands through the UART here
void shell(void)
{
    while(true)
    {
        USER_DATA data;
        bool valid = false;
        char proc_list[10][20] = {"Idle", "LengthyFn", "Flash4Hz", "OneShot", "ReadKeys", "Debounce", "Important", "Uncoop", "Errant", "Shell"};

        if(kbhitUart0())
        {
            getsUart0(&data);
            parseFields(&data);

            if(isCommand(&data, "reboot", 0) && (getFieldCount(&data) == 1))
            {
                valid = true;
                __asm("  SVC #12");
            }

            else if(isCommand(&data, "ps", 0) && (getFieldCount(&data) == 1))
            {
                valid = true;
                ps();
            }

            else if(isCommand(&data, "ipcs", 0) && (getFieldCount(&data) == 1))
            {
                valid = true;
                ipcs();
            }

            else if(isCommand(&data, "kill", 1))
            {
                char *arg = getFieldString(&data, 1);           //Gets the input as a string
                if(isInteger(arg) == true)                      //checks if input is an integer
                {
                    valid = true;
                    uint32_t pidN = getFieldInteger(&data, 1);
                    kill(pidN);
                }
                else
                {
                    putsUart0("Invalid input. Enter a process number.\n");
                }
            }

            else if(isCommand(&data, "pkill", 1))
            {
                char* procN = getFieldString(&data, 1);
                if(isInteger(procN) == true)
                {
                    valid = false;
                }
                else
                {
                    valid = true;
                    pkill(procN);
                }
            }

            else if(isCommand(&data, "pi", 1))
            {
                bool pi_status = false;
                char* status = getFieldString(&data, 1);
                if((status != NULL && cmpStr(status, "ON") == 0) || (status != NULL && cmpStr(status, "on") == 0))
                {
                    valid = true;
                    pi_status = true;
                }

                else if((status != NULL && cmpStr(status, "OFF") == 0) || (status != NULL && cmpStr(status, "off") == 0))
                {
                    valid = true;
                    pi_status = false;
                }

                if(valid)
                {
                    pi(pi_status);
                }
                else
                {
                    valid = false;
                }

            }

            else if(isCommand(&data, "preempt", 1))
            {

                bool preempt_status = false;
                char* status = getFieldString(&data, 1);
                if((status != NULL && cmpStr(status, "ON") == 0) || (status != NULL && cmpStr(status, "on") == 0))
                {
                    valid = true;
                    preempt_status = true;
                }
                else if((status != NULL && cmpStr(status, "OFF") == 0) || (status != NULL && cmpStr(status, "off") == 0))
                {
                    valid = true;
                    preempt_status = false;
                }

                if(valid)
                {
                    preempt(preempt_status);
                }
                else
                {
                    valid = false;
                }
            }

            else if(isCommand(&data, "sched", 1))
            {
                bool sched_status = false;
                char* schedule = getFieldString(&data, 1);
                if((schedule != NULL && cmpStr(schedule, "PRIO") == 0) || (schedule != NULL && cmpStr(schedule, "prio") == 0))
                {
                    valid = true;
                    sched_status = true;
                }

                else if((schedule != NULL && cmpStr(schedule, "RR") == 0) || (schedule != NULL && cmpStr(schedule, "rr") == 0))
                {
                    valid = true;
                    sched_status = false;
                }

                if(valid)
                {
                    sched(sched_status);
                }
                else
                {
                    valid = false;
                }
            }

            else if(isCommand(&data, "pidof", 1))
            {
                valid = true;
                char* proc_name = getFieldString(&data, 1);

                if(isInteger(proc_name) == true)
                {
                    valid = false;
                }
                else
                {
                    valid = true;
                    pidof(proc_name);
                }
            }

            else if(isCommand(&data, "meminfo", 0))
            {
                valid = true;
                meminfo();
            }

            else
            {
                if(getFieldCount(&data) == 1)
                {
                    char* proc_input = getFieldString(&data, 0);
                    uint8_t i = 0;
                    for(i = 0; i < 10; i++)
                    {
                        if(cmpStr(proc_input, proc_list[i]) == 0)  //compare the user input with the array list
                        {
                            valid = true;
                            restartThread((_fn)proc_input);
                        }
                    }
                }
                else
                {
                    valid = false;
                }
            }

            if(!valid)
            {
                putsUart0("Invalid command/process. Try again!\n");
            }
            putsUart0("\n\nRTOS>");
        }

        yield();
    }
}
