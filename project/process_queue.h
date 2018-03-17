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
    KERNEL_REQUEST_PARAM* request_param;
    PRIORITY priority;
    int arg;
    PID pid;
    struct ProcessDescriptor* next;
} PD;

typedef struct ProcessQueue {
    PD* front;
    PD* back;
    PRIORITY queue_type;
    unsigned int length;
} ProcessQ;
    
    
ProcessQ* Q_Init(ProcessQ* q, PRIORITY type);
void Q_Push(ProcessQ* q, PD* pd);
PD* Q_Pop(ProcessQ* q, PD* pd);
void Q_Insert(ProcessQ* q, PD* pd);


#endif
