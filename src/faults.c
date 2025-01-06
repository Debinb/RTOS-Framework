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
#include "tm4c123gh6pm.h"
#include "getInput.h"
#include "uart0.h"
#include "sp.h"
#include "faults.h"
#include "kernel.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: If these were written in assembly
//           omit this file and add a faults.s file

// REQUIRED: code this function
void mpuFaultIsr(void)
{
    uint32_t* psp = getPSP();
    uint32_t* msp = getMSP();
    uint32_t* PSD;
    PSD = psp;

    uint16_t pid = (uint16_t)(uintptr_t)PIDgetter();
    char str[50];

    putsUart0("\nMPU fault in process ");
    IntToStr(pid, str);
    putsUart0(str);
    putsUart0("\n");
    putsUart0("     PSP value:   0x");
    IntToHex((uint32_t)psp, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     MSP value:   0x");
    IntToHex((uint32_t)msp, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     MFAULT Flag: 0x");

    IntToHex((NVIC_FAULT_STAT_R & 0x000000FF), str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("Offending instruction at (PC): ");
    uint32_t PCValue = *(getPSP() + 6);
    IntToHex(PCValue, str);
    putsUart0(str);
    putsUart0(" and data address is ");
    //PCValue = *(getPSP() + 7);
    IntToHex((NVIC_MM_ADDR_R), str);
    putsUart0(str);
    putsUart0("\n");


    putsUart0("Process Stack Dump:\n");
    putsUart0("     R0:        0x");
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     R1:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     R2:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     R3:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     R12:       0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     LR:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     PC:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     xPSR:      0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    //Clears MPU fault pending bit
    NVIC_SYS_HND_CTRL_R &= ~(NVIC_SYS_HND_CTRL_MEMP);

    KillThread(pid);

    //Trigger PendSV ISR call
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;

}

// REQUIRED: code this function
void hardFaultIsr(void)
{
    uint32_t* psp = getPSP();
    uint32_t* msp = getMSP();
    uint32_t* PSD;
    PSD = psp;

    uint16_t pid = (uint16_t)(uintptr_t)PIDgetter();;
    char str[50];

    putsUart0("\nHard fault in process ");
    IntToStr(pid, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     PSP value:   0x");
    IntToHex((uint32_t)psp, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     MSP value:   0x");
    IntToHex((uint32_t)msp, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     MFAULT Flag: 0x");

    IntToHex((NVIC_FAULT_STAT_R & 0x000000FF), str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("Offending instruction at (PC): ");
    uint32_t PCValue = *(getPSP() + 6);
    IntToHex(PCValue, str);
    putsUart0(str);
    putsUart0("\n");


    putsUart0("Process Stack Dump:\n");
    putsUart0("     R0:        0x");
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     R1:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     R2:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     R3:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     R12:       0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     LR:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     PC:        0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    putsUart0("     xPSR:      0x");
    PSD++;
    IntToHex(*PSD, str);
    putsUart0(str);
    putsUart0("\n");

    while(1);
}

// REQUIRED: code this function
void busFaultIsr(void)
{
    uint16_t pid = (uint16_t)(uintptr_t)PIDgetter();;
    char str[20];

    putsUart0("Bus Fault in process ");
    IntToStr(pid, str);
    putsUart0(str);
    putsUart0("\n");

    while(1);
}

// REQUIRED: code this function
void usageFaultIsr(void)
{
    uint16_t pid = (uint16_t)(uintptr_t)PIDgetter();;
    char str[20];

    putsUart0("Usage Fault in process ");
    IntToStr(pid, str);
    putsUart0(str);
    putsUart0("\n");

    while(1);
}


