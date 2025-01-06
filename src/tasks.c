// Tasks
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
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "uart0.h"
#include "gpio.h"
#include "wait.h"
#include "kernel.h"
#include "tasks.h"
#include "mm.h"

//#define BLUE_LED   PORTF,2 // on-board blue LED
//#define RED_LED    PORTE,0 // off-board red LED
//#define ORANGE_LED PORTA,2 // off-board orange LED
//#define YELLOW_LED PORTA,3 // off-board yellow LED
//#define GREEN_LED  PORTA,4 // off-board green LED

#define BLUE_LED PORTF,2
#define BOARD_GREEN_LED PORTF,3
#define RED_LED PORTA,2
#define ORANGE_LED PORTA,3
#define YELLOW_LED PORTA,4
#define GREEN_LED PORTE,0

//Buttons Macros
#define PB0 PORTC,4
#define PB1 PORTC,5
#define PB2 PORTC,6
#define PB3 PORTC,7
#define PB4 PORTD,6
#define PB5 PORTD,7

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    // Setup LEDs and pushbuttons
    enablePort(PORTA);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTF);

    //Unlock special GPIO pin PD7
    setPinCommitControl(PB5);

    //Setting up the LED pins as output
    selectPinPushPullOutput(BLUE_LED);
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(ORANGE_LED);
    selectPinPushPullOutput(YELLOW_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(BOARD_GREEN_LED);

    //Setting Push buttons as input
    selectPinDigitalInput(PB0);
    selectPinDigitalInput(PB1);
    selectPinDigitalInput(PB2);
    selectPinDigitalInput(PB3);
    selectPinDigitalInput(PB4);
    selectPinDigitalInput(PB5);

    //Enables the internal pull up resistors for the push buttons
    enablePinPullup(PB0);
    enablePinPullup(PB1);
    enablePinPullup(PB2);
    enablePinPullup(PB3);
    enablePinPullup(PB4);
    enablePinPullup(PB5);

    //Enable Timer Clock
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;

    //System Handler Control enables Usage Fault Handler and Bus Fault Handler.
    //NVIC Configuration Control traps DIV0 and unaligned errors
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_USAGE | NVIC_SYS_HND_CTRL_BUS | NVIC_SYS_HND_CTRL_MEM;
    NVIC_CFG_CTRL_R |= NVIC_CFG_CTRL_DIV0;//| NVIC_CFG_CTRL_UNALIGNED;

    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);
}

// REQUIRED: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs(void)
{
    uint8_t indicator = 0;
    if(getPinValue(PB0) == 0)
    {
        indicator = 1;
    }
    else if(getPinValue(PB1) == 0)
    {
        indicator = 2;
    }
    else if(getPinValue(PB2) == 0)
    {
        indicator = 4;
    }
    else if(getPinValue(PB3) == 0)
    {
        indicator = 8;
    }
    else if(getPinValue(PB4) == 0)
    {
       indicator = 16;
    }
    else if(getPinValue(PB5) == 0)
    {
        indicator = 32;
    }
    return indicator;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while(true)
    {
        //lock(resource);
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        //unlock(resource);
        yield();
    }
}

void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    uint8_t *mem;
    mem = (uint8_t*)MallocWrapper(5000);
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
            mem[i] = i % 256;
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            stopThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}

void LedTimer(void)
{
    // Configure Timer 1 as the time base
     TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                     // turn-off timer before reconfiguring
     TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;               // configure as 32-bit timer (A+B)
     TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;              // configure for periodic mode (count down)
     TIMER1_TAILR_R = 40000000;                          // Timer Ticks # = Seconds x Clock Freq.
     TIMER1_IMR_R = TIMER_IMR_TATOIM;                     // turn-on interrupts for timeout in timer module
     TIMER1_CTL_R |= TIMER_CTL_TAEN;                      // turn-on timer
     NVIC_EN0_R = 1 << (INT_TIMER1A-16);                  // turn-on interrupt 37 (TIMER1A)
     setPinValue(BOARD_GREEN_LED, 1);
}

void TimerIsr(void)
{
    setPinValue(BOARD_GREEN_LED, 0);

}
