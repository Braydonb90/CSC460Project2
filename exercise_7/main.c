#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "LED_Test.h"

/**
 * \file shared.c
 * \brief A Skeleton Implementation of an RTOS
 * 
 * \mainpage A Skeleton Implementation of a "Self-Served" RTOS Model
 * This is an example of how to implement context-switching based on a 
 * self-served model. That is, the RTOS is implemented by a collection of
 * user-callable functions. The kernel executes its functions using the calling
 * task's stack.
 *
 * \author Dr. Mantis Cheng
 * \date 2 October 2006
 *
 * ChangeLog: Modified by Alexander M. Hoole, October 2006.
 *        -Rectified errors and enabled context switching.
 *        -LED Testing code added for development (remove later).
 *
 * \section Implementation Note
 * This example uses the ATMEL AT90USB1287 instruction set as an example
 * for implementing the context switching mechanism. 
 * This code is ready to be loaded onto an AT90USBKey.  Once loaded the 
 * RTOS scheduling code will alternate lighting of the GREEN LED light on
 * LED D2 and D5 whenever the correspoing PING and PONG tasks are running.
 * (See the file "cswitch.S" for details.)
 */

//Comment out the following line to remove debugging code from compiled version.
//#define DEBUG

typedef void (*voidfuncptr) (void);      /* pointer to void f(void) */ 

#define WORKSPACE     256
#define MAXPROCESS   4

#define BIT_RESET(PORT, PIN) (PORT &= ~(1<<PIN))
#define BIT_SET(PORT, PIN)   (PORT |= (1<<PIN))
#define BIT_SWAP(PORT, PIN) (PORT ^= (1<<PIN))
#define LOW_BYTE(X) (((uint16_t)X) & 0xFF)
#define HIGH_BYTE(X) ((((uint16_t)X) >> 8) & 0xFF)


/*--DEBUG----------------------*/

void debug_set_led(int state){ 
   BIT_SET(DDRB,4);
   if(state == 1)
       BIT_SET(PORTB, 4);
   else
       BIT_RESET(PORTB, 4);
}
int led_state = 0;
void debug_toggle_led(){
    led_state = !led_state;
    debug_set_led(led_state);
}

//flash led num times
//function blocks while flashing, only intended for debugging
void debug_flash_led(int num){
    int i;
    int delay = 200; 
    for(i = 0; i < num; i++){
        debug_set_led(1);
        _delay_ms(delay);
        debug_set_led(0);
        _delay_ms(delay);
    }
}




/*===========
  * RTOS Internal
  *===========
  */

/**
  * This internal kernel function is the context switching mechanism.
  * Fundamentally, the CSwitch() function saves the current task CurrentP's
  * context, selects a new running task, and then restores the new CurrentP's
  * context.
  * (See file "switch.S" for details.)
  */
extern void CSwitch();

/* Prototype */
void Task_Terminate(void);

/**
  * Exit_kernel() is used when OS_Start() or Task_Terminate() needs to 
  * switch to a new running task.
  */
extern void Exit_Kernel();

#define Disable_Interrupt()         asm volatile ("cli"::)
#define Enable_Interrupt()          asm volatile ("sei"::)

/**
  *  This is the set of states that a task can be in at any given time.
  */
typedef enum process_states 
{ 
   DEAD = 0, 
   READY, 
   RUNNING 
} PROCESS_STATES;


/**
  * Each task is represented by a process descriptor, which contains all
  * relevant information about this task. For convenience, we also store
  * the task's stack, i.e., its workspace, in here.
  * To simplify our "CSwitch()" assembly code, which needs to access the
  * "sp" variable during context switching, "sp" MUST BE the first entry
  * in the ProcessDescriptor.
  * (See file "cswitch.S" for details.)
  */
typedef struct ProcessDescriptor 
{
   unsigned char *sp;   
   unsigned char workSpace[WORKSPACE]; 
   PROCESS_STATES state;
} PD;

/**
  * This table contains ALL process descriptors. It doesn't matter what
  * state a task is in.
  */
static PD Process[MAXPROCESS];

/**
  * The process descriptor of the currently RUNNING task.
  */
  //??? Removed static because it was blocking external access.
  //??? Rename Cp to CurrentP because 'cp' is reserved in assembly.
volatile PD* CurrentP; 

volatile PD* KernelSp; 

/** index to next task to run */
volatile static unsigned int NextP;  

/** 1 if kernel has been started; 0 otherwise. */
volatile static unsigned int KernelActive;  

/** number of tasks created so far */
volatile static unsigned int Tasks;  


/**
 * When creating a new task, it is important to initialize its stack just like
 * it has called "CSwitch()"; so that when we switch to it later, we
 * can just restore its execution context on its stack.
 * (See file "cswitch.S" for details.)
 */
void Kernel_Create_Task_At( PD *p, voidfuncptr f ) 
{   
   unsigned char *sp;
#ifdef DEBUG
   int counter = 0;
#endif

   sp = (unsigned char *) &(p->workSpace[WORKSPACE-1]);

   /*----BEGIN of NEW CODE----*/
   //Initialize the workspace (i.e., stack) and PD here!

   //Clear the contents of the workspace
   memset(&(p->workSpace),0,WORKSPACE);

   //Notice that we are placing the address (16-bit) of the functions
   //onto the stack in reverse byte order (least significant first, followed
   //by most significant).  This is because the "return" assembly instructions 
   //(rtn and rti) pop addresses off in BIG ENDIAN (most sig. first, least sig. 
   //second), even though the AT90 is LITTLE ENDIAN machine.

   //Store terminate at the bottom of stack to protect against stack underrun.
   *(unsigned char *)sp-- = LOW_BYTE(Task_Terminate);
   *(unsigned char *)sp-- = HIGH_BYTE(Task_Terminate);
   *(unsigned char *)sp-- = LOW_BYTE(0);

   //Place return address of function at bottom of stack
   *(unsigned char *)sp-- = LOW_BYTE(f);
   *(unsigned char *)sp-- = HIGH_BYTE(f);
   *(unsigned char *)sp-- = LOW_BYTE(0);

   //Place stack pointer at top of stack
   sp = sp - 34;
      
   p->sp = sp;    /* stack pointer into the "workSpace" */

   /*----END of NEW CODE----*/



   p->state = READY;
}


/**
  *  Create a new task
  */
static void Kernel_Create_Task( voidfuncptr f ) 
{
   int x;

   if (Tasks == MAXPROCESS) return;  /* Too many task! */

   /* find a DEAD PD that we can use  */
   for (x = 0; x < MAXPROCESS; x++) {
       if (Process[x].state == DEAD) break;
   }

   ++Tasks;
   Kernel_Create_Task_At( &(Process[x]), f );
}

/**
  * This internal kernel function is a part of the "scheduler". It chooses the
  * next task to run, i.e., CurrentP.
  */
void Dispatch()
{
     /* find the next READY task
      * Note: if there is no READY task, then this will loop forever!.
      */
   while(Process[NextP].state != READY) {
      NextP = (NextP + 1) % MAXPROCESS;
   }


     /* we have a new CurrentP */
   CurrentP = &(Process[NextP]);
   CurrentP->state = RUNNING;
    
   int i = 0;
    
   //Moved to bottom (this was in the wrong place).
   NextP = (NextP + 1) % MAXPROCESS;
}


/*================
  * RTOS  API  and Stubs
  *================
  */

/*****
 *  Prototypes
 ****/

 void Setup_Timer();



/**
  * This function initializes the RTOS and must be called before any other
  * system calls.
  */
void OS_Init() 
{
   int x;

   Tasks = 0;
   KernelActive = 0;
   NextP = 0;

   for (x = 0; x < MAXPROCESS; x++) {
      memset(&(Process[x]),0,sizeof(PD));
      Process[x].state = DEAD;
   }
}


/**
  * This function starts the RTOS after creating a few tasks.
  */
void OS_Start() 
{   
   if ( (! KernelActive) && (Tasks > 0)) {
      Disable_Interrupt();

      /* here we go...  */
      KernelActive = 1;


      debug_toggle_led();
      _delay_ms(1000);
      //This shouldn't return here but it do
      Exit_Kernel();
   }
}


/**
  * For this example, we only support cooperatively multitasking, i.e.,
  * each task gives up its share of the processor voluntarily by calling
  * Task_Next().
  */
void Task_Create( voidfuncptr f)
{
   Disable_Interrupt();
   Kernel_Create_Task( f );
   Enable_Interrupt();
}

/**
  * The calling task gives up its share of the processor voluntarily.
  */
void CSwitch_Test(){
    CSwitch();
}
void Task_Next() 
{
   if (KernelActive) {
     Disable_Interrupt();
     CurrentP->state = READY;
     BIT_SET(PORTB,4);
     CSwitch_Test();
     /* resume here when this task is rescheduled again later */
     Enable_Interrupt();
     BIT_RESET(PORTB,4);
  }
}


/**
  * The calling task terminates itself.
  */
void Task_Terminate() 
{
   if (KernelActive) {
      Disable_Interrupt();
      CurrentP -> state = DEAD;
        /* we will NEVER return here! */
      asm ( "jmp Exit_Kernel":: );
   }
}

void Setup_Timer() 
{

    TCCR4A = 0;
    TCCR4B = 0;  
    //Set to CTC (mode 4)
    TCCR4B |= (1<<WGM42);

    //Set prescaller to 256
    TCCR4B |= (1<<CS42);

    //Set TOP value (0.5 seconds)
    OCR4A = 32500;

    //Enable interupt A for timer 4.
    TIMSK4 |= (1<<OCIE4A);

    //Set timer to 0 (optional here).
    TCNT4 = 0;
  
}

ISR(TIMER4_COMPA_vect)
{
 // debug_toggle_led();
  Task_Next();
}




/*============
  * A Simple Test 
  *============
  */

/**
  * A cooperative "Ping" task.
  * Added testing code for LEDs.
  */
void Ping() 
{
    Enable_Interrupt();
    int  x,y;
    BIT_SET(DDRA, 6);
    for(;;){
        //LED on
        BIT_SET(PORTA, 6);
        _delay_ms(500);
        debug_toggle_led();
        BIT_RESET(PORTA, 6);
        _delay_ms(500);
    }
}


/**
  * A cooperative "Pong" task.
  * Added testing code for LEDs.
  */
void Pong() 
{
    Enable_Interrupt();
    int  x,y;
    BIT_SET(DDRB, 6);
    for(;;) {
        //LED on
        BIT_SET(PORTB, 6);
        _delay_ms(500);
        BIT_RESET(PORTB, 6);
        _delay_ms(500);
    }
}


/**
  * This function creates two cooperative tasks, "Ping" and "Pong". Both
  * will run forever.
  */
int main() 
{
    OS_Init();
    Task_Create( Pong );
    Task_Create( Ping );
    Setup_Timer();

    OS_Start();

    for( ;; );

    return 0;
}

