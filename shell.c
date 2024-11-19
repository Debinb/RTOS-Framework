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
#include "getInput.h"
#include "gpio.h"
#include "uart0.h"
#include "shell.h"
#include "kernel.h"
#include "sp.h"

// REQUIRED: Add header files here for your strings functions, ...

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


void ps()
{
    putsUart0("PS called.\n");
}

void ipcs()
{
    putsUart0("IPCS called.\n");
}

void kill(uint32_t pid)         //StopThread()
{
    stopThread((_fn)pid);
    char str[100];
    IntToStr(pid, str);
    putsUart0(str);
    putsUart0(" killed\n");
}

void pkill(char* proc_name)     //StopThread()
{
    stopThread((_fn)proc_name);
    putsUart0(proc_name);
    putsUart0(" killed\n");
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
//    char str[15];
//
//    uint32_t pidValue = ReadFromR1();
//
//    IntToStr(pidValue, str);
//    putsUart0(name);
//    putsUart0(" launched. \nPID: ");
//    putsUart0(str);
//    putcUart0('\n');
}


// REQUIRED: add processing for the shell commands through the UART here
void shell(void)
{
    while(true)
    {
        USER_DATA data;
        bool valid = false;
        char proc_list[3][15] = {"idle", "wait", "update"};

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

            else
            {
                if(getFieldCount(&data) == 1)
                {
                    char* proc_input = getFieldString(&data, 0);
                    uint8_t i = 0;
                    for(i = 0; i < 3; i++)
                    {
                        if(cmpStr(proc_input, proc_list[i]) == 0)  //compare the user input with the array list
                        {
                            valid = true;
                            putsUart0(proc_input);
                            putsUart0(" running.\n");
                            setPinValue(PORTF, 3, 1);
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
                putsUart0("Invalid command.\n");
                setPinValue(PORTF, 3, 0);
            }
            putsUart0("\n\n>");
        }

        yield();
    }
}
