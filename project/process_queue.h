#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H


/**
  * Each task is represented by a process descriptor, which contains all
  * relevant information about this task. For convenience, we also store
  * the task's stack, i.e., its workspace, in here.
  */
typedef struct ProcessDescriptor 
{
   unsigned char *sp;   /* stack pointer into the "workSpace" */
   unsigned char workSpace[WORKSPACE]; 
   PROCESS_STATES state;
   voidfuncptr  code;   /* function to be executed as a task */
   KERNEL_REQUEST_TYPE request;
} PD;


#endif
