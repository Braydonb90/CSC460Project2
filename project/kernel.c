#include <string.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include "kernel.h"

#define DEBUG_LED_PIN 4

/*===========
 * RTOS Internal
 *===========
 */

/*
 * This table contains ALL process descriptors. It doesn't matter what
 * state a task is in.
 */
static PD Process[MAXTHREAD];

/*
 * The process descriptor of the currently RUNNING task.
 */
volatile static PD* Cp; 
volatile static KERNEL_REQUEST_PARAM* current_request;

/* 
 * Since this is a "full-served" model, the kernel is executing using its own
 * stack. We can allocate a new workspace for this kernel stack, or we can
 * use the stack of the "main()" function, i.e., the initial C runtime stack.
 * (Note: This and the following stack pointers are used primarily by the
 *   context switching code, i.e., CSwitch(), which is written in assembly
 *   language.)
 */         
volatile unsigned char *KernelSp;

/*
 * This is a "shadow" copy of the stack pointer of "Cp", the currently
 * running task. During context switching, we need to save and restore
 * it into the appropriate process descriptor.
 */
volatile unsigned char *CurrentSp;

/* index to next task to run */
volatile static unsigned int NextP;  

/* 1 if kernel has been started; 0 otherwise. */
volatile static unsigned int KernelActive;  

/* number of tasks created so far */
volatile static unsigned int Tasks;  

static ProcessQ periodic_q;
static ProcessQ system_q;
static ProcessQ rr_q;

/*
 * (See file "cswitch.S" for details.)
 */
void Kernel_Create_Task_At( PD *p, voidfuncptr f ) 
{   
   unsigned char *sp;

   sp = (unsigned char *) &(p->workSpace[WORKSPACE-1]);

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
     
   p->sp = sp;		/* stack pointer into the "workSpace" */
   p->code = f;		/* function to be executed as a task */
   p->request_param = NULL;

   p->state = READY;

}


/*
 *  Create a new task
 */
static void Kernel_Create_Task() 
{
    int x;

    if (Tasks == MAXTHREAD) return;  /* Too many task! */

    /* find a DEAD PD that we can use  */
    for (x = 0; x < MAXTHREAD; x++) {
        if (Process[x].state == DEAD) break;
    }

    if(x < MAXTHREAD) {
        Kernel_Create_Task_At(Process + x, current_request->code);
        
        Process[x].arg = current_request->arg;
        Process[x].priority = current_request->priority;

        switch (Process[x].priority){
            case SYSTEM:
                Q_Push(&system_q, Process + x);
                break;
            case PERIODIC:
                Q_Insert(&periodic_q, Process + x);
                break;
            case RR:
                Q_Push(&rr_q, Process + x);
                break;
            default:
                OS_Abort(INVALID_REQUEST);
        }
    }
    else {
        OS_Abort(NO_DEAD_PDS);
    }
    ++Tasks;

}


/*
 * This internal kernel function is a part of the "scheduler". It chooses the 
 * next task to run, i.e., Cp.
 */
static void Dispatch()
{


   
   Cp = &(Process[NextP]);
   CurrentSp = Cp->sp;
   Cp->state = RUNNING;

   NextP = (NextP + 1) % MAXTHREAD;
}

/*
 * This is the main loop of our kernel, called by Kernel_Start().
 */
static void Kernel_Next_Request() 
{
   Dispatch();  /* select a new task to run */

    while(1) {
        Cp->request_param = NONE; /* clear its request */

        /* activate this newly selected task */
        CurrentSp = Cp->sp;
        Exit_Kernel();    /* The task will be running after this */

        /* if this task makes a system call, it will return to here! */

        /* save the Cp's stack pointer */
        Cp->sp = CurrentSp;
        switch(current_request->request_type){
        case CREATE:
            Kernel_Create_Task();
            break;
        case NEXT:
        case NONE:
            /* NONE could be caused by a timer interrupt */
            Cp->state = READY;
            Dispatch();
            break;
        case TERMINATE:
            /* deallocate all resources used by this task */
            Cp->state = DEAD;
            Dispatch();
            break;
        default:
            /* Houston! we have a problem here! */
            break;
        }
    } 
}

//The only "public" function
void Kernel_Request(KERNEL_REQUEST_PARAM* krp) {
    if(KernelActive) {
        Disable_Interrupt();
        //why am I storing this???
        Cp->request_param = krp;
        current_request = krp;

        Enter_Kernel();
    }
}

/*================
 * Interrupt Stuff
 *================
 */


static void Setup_System_Clock() 
{

    TCCR4A = 0;
    TCCR4B = 0;  
    //Set to CTC (mode 4)
    TCCR4B |= (1<<WGM42);

    //Set prescaler to 256 (16000000 / 256 = 62500)
    TCCR4B |= (1<<CS42);

    //Set TOP value (0.01 seconds)
    OCR4A = (int)(62500 * ((float)MSECPERTICK / 1000));

    //Enable interupt A for timer 4.
    TIMSK4 |= (1<<OCIE4A);

    //Set timer to 0 (optional here).
    TCNT4 = 0;
  
}

ISR(TIMER4_COMPA_vect)
{
    //TODO
}


/*================
 * RTOS  API  and Stubs
 *================
 */

/*
 * This function initializes the kernel and must be called before any other
 * system calls.
 */
void Kernel_Init() 
{
   int x;

   Q_Init(&system_q, SYSTEM);
   Q_Init(&periodic_q, PERIODIC);
   Q_Init(&rr_q, RR);

   Tasks = 0;
   KernelActive = 0;
   NextP = 0;

	//Reminder: Clear the memory for the task on creation.
   for (x = 0; x < MAXTHREAD; x++) {
      memset(&(Process[x]),0,sizeof(PD));
      Process[x].state = DEAD;
   }
}

  
/*
 * This function starts the RTOS after creating a few tasks.
 */
void Kernel_Start() 
{   
   if ( (! KernelActive) && (Tasks > 0)) {
       Disable_Interrupt();
      /* we may have to initialize the interrupt vector for Enter_Kernel() here. */

      /* here we go...  */
      KernelActive = 1;
      Kernel_Next_Request();
      /* NEVER RETURNS!!! */
   }
} 

/*
 * This function creates two cooperative tasks, "Ping" and "Pong". Both
 * will run forever.
 */
void main() 
{
   Kernel_Init();
   Setup_System_Clock();
   Kernel_Start();
}

