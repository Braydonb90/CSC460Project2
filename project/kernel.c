#include <string.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include "kernel.h"


/*===========
 * RTOS Internal
 *===========
 */


/****Prototypes****/

/*
 * When creating a new task, it is important to initialize its stack just like
 * it has called "Enter_Kernel()"; so that when we switch to it later, we
 * can just restore its execution context on its stack.
 * (See file "cswitch.S" for details.)
 */
static void Kernel_Create_Task_At( PD *p, voidfuncptr f ); 

/**
  *  Create a new task
  */
static void Kernel_Create_Task();

/**
  * This internal kernel function is a part of the "scheduler". It chooses the 
  * next task to run, i.e., Cp.
  */
static void Dispatch();

/*
 * This internal kernel function is the "main" driving loop of this full-served
 * model architecture. Basically, on Kernel_Start(), the kernel repeatedly
 * requests the next user task's next system call and then invokes the
 * corresponding kernel function on its behalf.
 *
 * This is the main loop of our kernel, called by Kernel_Start().
 */
static void Kernel_Next_Request();

/*
 * Task has requested to be terminated. Need to clear mem and set to DEAD
 */
static void Kernel_Request_Terminate();

/*
 * This function initializes the kernel and must be called before any other
 * system calls.
 */
static void Kernel_Init();

/*
 * This function starts the RTOS after creating a few tasks.
 */
static void Kernel_Start(); 

/*
 * Set up the System Clock. An interrupt is called every tick using this clock
 */
static void Setup_System_Clock(); 

/*
 * System idle task
 */
void Kernel_Idle_Task();

void debug_break(int argcount, ...);
/****End of Prototypes****/

/****Other Things****/

/*
 * This table contains ALL process descriptors. It doesn't matter what
 * state a task is in.
 */
static PD Process[MAXTHREAD];

/*
 * Process descriptor for the idle task
 */
static PD idle_process;

/*
 * The process descriptor of the currently RUNNING task.
 */
volatile static PD* Cp; 

/*
 * The currently assigned request info, waiting to be processed
 * Requests aren't always associated with a task, so need to have this
 */
static KERNEL_REQUEST_PARAM current_request;

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


/****Start of Implementation****/

/*
 * (See file "cswitch.S" for details.)
 */
static void Kernel_Create_Task_At( PD *p, voidfuncptr f ) 
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
   p->request_param.request_type = NONE;

   p->state = READY;
   Tasks++;
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
        Kernel_Create_Task_At(Process + x, current_request.code);
        
        Process[x].arg = current_request.arg;
        Process[x].priority = current_request.priority;
        Process[x].pid = x;

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
                OS_Abort(INVALID_PRIORITY_CREATE);
        }
    }
    else {
        OS_Abort(NO_DEAD_PDS);
    }
}


/*
 * This internal kernel function is a part of the "scheduler". It chooses the 
 * next task to run, i.e., Cp.
 */
static void Dispatch()
{
    //put current process back in queue, if relevant
    //if the request was terminate, Cp should already be dead    
    if (Cp->state != DEAD) {
        if(current_request.request_type == TERMINATE) {
            OS_Abort(INVALID_TERMINATE);
        }
        Cp->state = READY;
        switch(Cp->priority) {
            case SYSTEM:
                Q_Push(&system_q, (PD*)Cp);
                break;
            case PERIODIC:
                Q_Insert(&periodic_q, (PD*)Cp);
                break;
            case RR:
                Q_Push(&rr_q, (PD*)Cp);
                break;
            case -1:
                break;
            default:
                OS_Abort(INVALID_PRIORITY_DISPATCH);
                break;
        }
    } 
    //find new process 
    if (system_q.length > 0) {
        Cp = Q_Pop(&system_q);
        Cp->state = RUNNING;
    }
    else if(periodic_q.length > 0) {
        Cp = Q_Pop(&periodic_q);
        Cp->state = RUNNING;
    }
    else if(rr_q.length > 0) {
        Cp = Q_Pop(&rr_q);
        Cp->state = RUNNING;
    }
    else {
        Cp = &idle_process;
    }
}

/*
 * This is the main loop of our kernel, called by Kernel_Start().
 */
static void Kernel_Next_Request() 
{
   Dispatch();  /* select a new task to run */

    while(1) {
        Cp->request_param.request_type = NONE; /* clear its request */

        /* activate this newly selected task */
        CurrentSp = Cp->sp;
        Exit_Kernel();    /* The task will be running after this */

        /* if this task makes a system call, it will return to here! */

        /* save the Cp's stack pointer */
        Cp->sp = CurrentSp;
       // Blink_Pin(DEBUG_PIN, current_request.request_type);

       // _delay_ms(500);
        switch(current_request.request_type){
            case CREATE:
                Kernel_Create_Task();
                break;
            case NEXT:
            case NONE:
                /* NONE could be caused by a timer interrupt --- this is no longer true */
                Cp->state = READY;
                Dispatch();
                break;
            case TERMINATE:
                /* deallocate all resources used by this task */
                Kernel_Request_Terminate();
                Dispatch();
                break;
            case TIMER_TICK:
                //Only time we do anything is if task is RR?
                Dispatch();
                break;
            default:
                /* Houston! we have a problem here! */
                OS_Abort(INVALID_REQUEST);
                break;
        }
        current_request.request_type = NONE;
    } 
}


/*
 * Task has requested to be terminated. Need to clear mem and set to DEAD
 */
static void Kernel_Request_Terminate() {
    //This cast shushes compiler. Assuming it's ok?
    memset((PD*)Cp, 0, sizeof(PD));
    Cp->state = DEAD;
    Tasks--;
}
    
//The only "public" function
void Kernel_Request(KERNEL_REQUEST_PARAM krp) {
    if(KernelActive) {
        Disable_Interrupt();
        //why am I storing this???
        //to pass values back
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

/*
 * Called once every system tick
 */
ISR(TIMER4_COMPA_vect)
{
    if (KernelActive) {
        BIT_TOGGLE(PORTB, CLOCK_PIN);

        // Need to indicate that this is just a tick, for the likely case that 
        // the Cp doesn't need to get switched
        // There should not be any current request at this point
        // but if there is????
        KERNEL_REQUEST_PARAM prm;
        prm.request_type = TIMER_TICK; 

        current_request = prm;

        Enter_Kernel();
    }
        
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
    Tasks = 0;
    KernelActive = 0;
    NextP = 0;

    BIT_SET(DDRB, DEBUG_PIN);
    BIT_SET(DDRB, CLOCK_PIN);
    BIT_SET(DDRB, ERROR_PIN);

    Q_Init(&system_q, SYSTEM);
    Q_Init(&periodic_q, PERIODIC);
    Q_Init(&rr_q, RR);

    //Create the user main task request
    KERNEL_REQUEST_PARAM prm;
    prm.request_type = CREATE;
    prm.priority = SYSTEM;
    prm.code = user_main;
    prm.arg = 0;

    //Create system idle process
    //Shouldn't need to do any initialization? 
    memset(&idle_process, 0, sizeof(PD));
    Kernel_Create_Task_At(&idle_process, Kernel_Idle_Task);
    idle_process.priority = -1;



    //Reminder: Clear the memory for the task on creation.
    for (x = 0; x < MAXTHREAD; x++) {
        memset(&(Process[x]),0,sizeof(PD));
        Process[x].state = DEAD;
    }
    current_request = prm;
}

  
/*
 * This function starts the RTOS after creating a few tasks.
 */
void Kernel_Start() 
{   
    if ( (! KernelActive) && (Tasks > 0)) {
        Disable_Interrupt();

        /* here we go...  */
        KernelActive = 1;
        Kernel_Next_Request();
        /* NEVER RETURNS!!! */
     }
} 

/*
 * The idle task for the OS. Has the lowest priority, runs whenever nothing else is being run
 */
void Kernel_Idle_Task() {
    for (;;) {
        //TODO Toggle some pin here
        if(current_request.request_type != NONE) {
            Enter_Kernel();
        }
    }
}


void main() 
{
    Kernel_Init();
    Setup_System_Clock();
    Kernel_Start();
}

