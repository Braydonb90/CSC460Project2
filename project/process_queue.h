#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H

#include "common.h"

/**
  * Each task is represented by a process descriptor, which contains all
  * relevant information about this task. For convenience, we also store
  * the task's stack, i.e., its workspace, in here.
  */
typedef struct ProcessDescriptor 
{
    volatile unsigned char *sp;   /* stack pointer into the "workSpace" */
    unsigned char workSpace[WORKSPACE]; 
    volatile PROCESS_STATE state;
    voidfuncptr  code;   /* function to be executed as a task */
    PRIORITY priority;
    int arg;
    PID pid;
    KERNEL_REQUEST_PARAM request_param; //Any reason to store this here?
    struct ProcessDescriptor* next;
	
	// Only used for periodic tasks
	TICK remaining; //remaining allowed execution time
	TICK next_start;//tick at which this task is next scheduled
	
    TICK wcet;
    TICK period;
    TICK offset;
	
} PD;

typedef struct ProcessQueue {
    PD* front;
    PD* back;
    PRIORITY queue_type;
    unsigned int length;
} ProcessQ;
    
    
ProcessQ* Q_Init(ProcessQ* q, PRIORITY type);
void Q_Push(ProcessQ* q, PD* pd);
PD* Q_Pop_Ready(ProcessQ* q);
PD* Q_Pop(ProcessQ* q);
void Q_Insert(ProcessQ* q, PD* pd);


#endif
