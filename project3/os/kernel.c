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
static void Kernel_Request_Msg_Send();
static void Kernel_Request_Msg_Recv();
static void Kernel_Request_Msg_Reply();

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
static KERNEL_REQUEST_PARAM* current_request;
static KERNEL_REQUEST_PARAM  current_request_copy;

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

/* number of Ticks since system start */
/* at 10ms per tick, should take ~497 days to overflow */ 
static TICK Elapsed;

static ProcessQ periodic_q;
static ProcessQ system_q;
static ProcessQ rr_q;

BOOL idling;
BOOL do_break = FALSE;

char* Get_State(short s) {
	switch(s) {
		case DEAD:				return "DEAD";
		case READY:				return "READY";
		case RUNNING:			return "RUNNING";
		case BLOCKED_SEND:		return "BLOCKED_SEND";
		case BLOCKED_RECEIVE:	return "BLOCKED_RECEIVE";
		case BLOCKED_REPLY:		return "BLOCKED_REPLY";
		default:				return "NOT FOUND";
	}
}

char* Get_Request_Type(short s) {
	switch(s) {
		case NONE:		return "NONE";
		case CREATE:	return "CREATE";
		case NEXT:		return "NEXT";
		case TIMER_TICK:return "TIMER_TICK";
		case TERMINATE:	return "TERMINATE";
		case SEND:		return "SEND";
		case RECEIVE:	return "RECEIVE";
		case REPLY:		return "REPLY";
		default:		return "NOT FOUND";
	}
}

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
        Kernel_Create_Task_At(Process + x, current_request->code);
        
        Process[x].arg = current_request->arg;
        Process[x].priority = current_request->priority;
        Process[x].pid = x;
        
        //need to pass back pid. PD holds copy of param struct for safety reasons
        //so current_request pointer needs to be assigned to
        current_request->pid = x;
        
        switch (Process[x].priority){
            case SYSTEM:
                Q_Push(&system_q, Process + x);
                break;
            case PERIODIC:
				Process[x].wcet = current_request->wcet;
				Process[x].period = current_request->period;
				Process[x].next_start = Elapsed + current_request->offset;
				Process[x].state = READY;
                if(periodic_q.length == 2)
                    do_break = TRUE;
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
    if(DEBUG) puts("dispatch\n");

    if(Cp != NULL){
		//put current process back in queue, if relevant
		//if the request was terminate, Cp should already be dead    
        switch(Cp->priority) {
            case SYSTEM:
				if(Cp->state == RUNNING) {
					Cp->state = READY;
				}
                //only push system task if it isn't dead
                if(Cp->state != DEAD) {
                    Q_Push(&system_q, (PD*)Cp);
                }
                /*else if (Cp->state == DEAD){
                    break;
                }
                else{
                    OS_Abort(INVALID_STATE_DISPATCH);
                }*/
                break;
            case PERIODIC:
                Q_Insert(&periodic_q, (PD*)Cp);
                break;
            case RR:
            // RR Task needs to be at beginning of queue if preempted
				if(Cp->state == RUNNING) {
                    Cp->state = READY;
				}
                if(Cp->state != DEAD){
                    Q_Push(&rr_q, (PD*)Cp);
                }
                break;
            case IDLE:
                break;
            default:
                OS_Abort(INVALID_PRIORITY_DISPATCH);
                break;
        }
    }
    idling = FALSE;
    Cp = NULL;
    //find new process 
    if (system_q.length > 0) {
        Cp = Q_Pop_Ready(&system_q);
       /* if(Cp == NULL) {
            // This shouldnt be queue error now
            OS_Abort(QUEUE_ERROR);
        }*/
    }
    if(Cp == NULL && periodic_q.length > 0) {
        int r_count = Q_CountScheduledTasks(&periodic_q, Elapsed);

        if(r_count > 1){
            OS_Abort(TIMING_VIOLATION);
        }
        else if(r_count == 1){
            Cp = Q_Pop(&periodic_q);
        }
    }
    if(Cp == NULL && rr_q.length > 0) {
        Cp = Q_Pop_Ready(&rr_q);
    }
    
    if(Cp == NULL) {
        idling = TRUE;
        Cp = &idle_process;
    } else {
        Cp->state = RUNNING;
	}
    if(!idling) {
        BIT_RESET(OUTPUT_PORT, IDLE_PIN);
    }
}

/*
 * This is the main loop of our kernel, called by Kernel_Start().
 */
 int num_next = 0;
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
        if(DEBUG) printf("Kernel_Next_Request Info:\nType: %d | Priority: %d\n", 
                                        current_request->request_type, current_request->priority);
        Cp->sp = CurrentSp;
        if(current_request == NULL) {
            OS_Abort(NULL_REQUEST);
        }
        if(do_break)
            debug_break(3,1,2,1);
        switch(current_request_copy.request_type){
            case CREATE:
                Kernel_Create_Task();
                break;
            case NEXT:
                if(Cp->priority == PERIODIC) {
                    Cp->next_start = Cp->next_start + Cp->period;
                    Cp->state = READY;
                }
                num_next++;
                if(DEBUG) printf("Next calls: %d\n", num_next);
                Cp->state = READY;
                Dispatch();
                break;
            case TERMINATE:
                /* deallocate all resources used by this task */
                Kernel_Request_Terminate();
                Dispatch();
                break;
            case TIMER_TICK:
                //SYSTEM should never need an interrupt to switch (some timeout is a good idea?)
                //PERIODIC should update here, and dispatch if past wcet
                //RR is lowest priority, so dispatch immediately here
                if(idling || Cp->priority == RR) { 
                    Dispatch();
                }
                else if(Cp->priority == PERIODIC){
                    if(Cp->next_start + Cp->wcet <= Elapsed){
                        OS_Abort(PERIODIC_OVERUSE);
                    }
                }
                else {
                    if(DEBUG) printf("system_q: %d\nperiodic_q: %d\nrr_q: %d\n\nCp priority: %d\nCp PID: %d\n", 
                                system_q.length, periodic_q.length, rr_q.length,
                                Cp->priority, Cp->pid);
                }
                break;
            case SEND:
				if(DEBUG) printf("SEND\n");
				if(Cp->priority == PERIODIC) {
					OS_Abort(INVALID_MSG_SEND_REQUEST);
				}
				Kernel_Request_Msg_Send();
				Dispatch();
				break;
			case RECEIVE:
				if(DEBUG) printf("Receive Mask: %d\n", current_request->msg_detail.mask);
				if(Cp->priority == PERIODIC) {
					OS_Abort(INVALID_MSG_RECEIVE_REQUEST);
				}
				Kernel_Request_Msg_Recv();
				Dispatch();
				break;
			case REPLY:
				if(DEBUG) printf("Reply\n");
				if(Cp->priority == PERIODIC) {
					OS_Abort(INVALID_MSG_REPLY_REQUEST);
				}
				Kernel_Request_Msg_Reply();
				Dispatch();
				break;
            default:
                /* Houston! we have a problem here! */
                if(DEBUG) printf("request type: %d\n", current_request_copy.request_type);
                OS_Abort(INVALID_REQUEST);
                break;
        }
        current_request = NULL;

        if(DEBUG) printf("exiting kernel\n");
    } 
}

int Kernel_GetArg() {
    return Cp->arg;
}
PID Kernel_GetPid() {
    return Cp->pid;
}

TICK Kernel_GetElapsed() {
	return Elapsed;
}

static void Kernel_Request_Msg_Send(){
        if(current_request->msg_detail.pid < MAXTHREAD && Process[current_request->msg_detail.pid].state != DEAD) {
					Cp->msg_detail.pid = current_request->msg_detail.pid;
					Cp->msg_detail.msg = current_request->msg_detail.msg;
					Cp->msg_detail.type = current_request->msg_detail.type;
					Cp->state = BLOCKED_SEND;
					int x;
					for(x = 0; x < MAXTHREAD; x++) { // Check to see if any messages are blocked by receive
					
						if(Process[x].state == BLOCKED_RECEIVE) {
							if(DEBUG) printf("Found: %d\n", Process[x].pid);
							if(DEBUG) printf("Message: %d\n", Cp->msg_detail.pid);
							if(DEBUG) printf("Mask: %d\n", (Process[x].msg_detail.mask));
							if(DEBUG) printf("Type: %d\n",  Cp->msg_detail.type);
							
							if(Process[x].pid == Cp->msg_detail.pid && (Process[x].msg_detail.mask & Cp->msg_detail.type))
							{
								current_request->msg_detail.pid = Process[x].msg_detail.pid = Cp->pid;
								*(Cp->msg_detail.msg) = *(Process[x].msg_detail.msg);
								Cp->state = BLOCKED_REPLY;
								Process[x].state = READY;
								break;
							}
						}
					}
				} else {
					// NO MATCHING PID OR OUT OF RANGE
				}
}
static void Kernel_Request_Msg_Recv(){
current_request->msg_detail.pid = 0;
				Cp->state = BLOCKED_RECEIVE;
				
				Cp->msg_detail.mask = current_request->msg_detail.mask;
				int x;
				for(x = 0; x < MAXTHREAD; x++) { // Check to see if any messages are blocked by send for the current process
					if(Process[x].state == BLOCKED_SEND && Process[x].msg_detail.pid == Cp->pid && (Process[x].msg_detail.type & Cp->msg_detail.mask)) {
				BIT_SET(SYSTEM_PORT, 1);
						current_request->msg_detail.pid = Cp->msg_detail.pid = Process[x].pid;
						
						*(Cp->msg_detail.msg) = *(Process[x].msg_detail.msg);
						Process[x].state = BLOCKED_REPLY;
						Cp->state = READY;
						break;
					}
				}
}
static void Kernel_Request_Msg_Reply(){
if(Process[Cp->msg_detail.pid].state == BLOCKED_REPLY) {
					
					*(Process[Cp->msg_detail.pid].msg_detail.msg) = current_request->msg_detail.r;
					Process[Cp->msg_detail.pid].state = READY;
				}
}

/*
 * Task has requested to be terminated. Need to clear mem and set to DEAD
 */
static void Kernel_Request_Terminate() {
    // periodic tasks are rescheduled after termination
    if(Cp->priority != PERIODIC){
        //This cast shushes compiler. Assuming it's ok?
        memset((PD*)Cp, 0, sizeof(PD));
        Tasks--;
        Cp->state = DEAD;
    }
}
    
//The main interface between the kernel and the user level
void Kernel_Request(KERNEL_REQUEST_PARAM* krp) {
    if(KernelActive) {
        Disable_Interrupt();
        Cp->request_param = *krp;
        current_request = krp;
        current_request_copy = *krp;

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

    // Set TOP value (0.01 seconds)
    // OCR register is 16 bit, so max value is 65536
    // So we'll have our max period as 1 sec
    unsigned int per_tick = 62500 * ((MSECPERTICK>1000)? 1000 : MSECPERTICK) / 1000;

    if(DEBUG) printf("ms per system tick: %u\nclock ticks per system tick: %u\n", MSECPERTICK, per_tick); 
    _delay_ms(2000);
    OCR4A = per_tick;

    //Enable interupt A for timer 4.
    TIMSK4 |= (1<<OCIE4A);

    //Set timer to 0 (optional here).
    TCNT4 = 0;
  
}

/*
 * Called once every system tick
 */
static KERNEL_REQUEST_PARAM prm;
ISR(TIMER4_COMPA_vect)
{
    if (KernelActive) {
        if(DEBUG) puts("-------- START TICK ---------\n");
        BIT_TOGGLE(OUTPUT_PORT, CLOCK_PIN);
		Elapsed++;
		
        // Need to indicate that this is just a tick, for the likely case that 
        // the Cp doesn't need to get switched
        // There should not be any current request at this point
        // but if there is????
        // Shouldn't be possible, as any request should immediately disable interrupts
        if(current_request != NULL) {
            if(DEBUG) printf("current_request not null\n");
            OS_Abort(NON_NULL_REQUEST);
        }
        prm.request_type = TIMER_TICK; 
        prm.priority = SYSTEM;
        current_request_copy = prm;

        //Is this going out of scope???
        Kernel_Request(&prm);
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
    Elapsed = 0;
    idling = FALSE;
    KernelActive = 0;
    NextP = 0;

    BIT_SET(OUTPUT_PORT_INIT, IDLE_PIN);
    BIT_SET(OUTPUT_PORT_INIT, CLOCK_PIN);
    BIT_SET(OUTPUT_PORT_INIT, ERROR_PIN);
    BIT_SET(OUTPUT_PORT_INIT, DEBUG_PIN);

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
    idle_process.priority = IDLE;

    //Reminder: Clear the memory for the task on creation.
    for (x = 0; x < MAXTHREAD; x++) {
        memset(&(Process[x]),0,sizeof(PD));
        Process[x].state = DEAD;
    }
    current_request = &prm;
    Kernel_Create_Task();
    current_request = NULL;
}

  
/*
 * This function starts the RTOS
 */
void Kernel_Start() 
{   
    if ( (! KernelActive) && (Tasks > 0)) {
        Disable_Interrupt();

        /* here we go...  */
        KernelActive = TRUE;
        Kernel_Next_Request();
        /* NEVER RETURNS!!! */
     }
} 

/*
 * The idle task for the OS. Has the lowest priority, runs whenever nothing else is being run
 */
void Kernel_Idle_Task() {
    for (;;) {
        idling = TRUE;
        BIT_TOGGLE(OUTPUT_PORT, IDLE_PIN);
        _delay_ms(200);
    }
}


void main() 
{
	uart_init_debug();
    stdout = &uart_output;
    stdin  = &uart_input;
	
    Kernel_Init();
    Setup_System_Clock();
    Kernel_Start();
}

