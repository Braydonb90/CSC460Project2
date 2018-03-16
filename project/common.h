#ifndef COMMON_H
#define COMMON_H


/****TYPEDEFS*********/
/*
  *  This is the set of states that a task can be in at any given time.
  */
typedef enum process_states 
{ 
   DEAD = 0, 
   READY, 
   RUNNING 
} PROCESS_STATES;

/**
  * This is the set of kernel requests, i.e., a request code for each system call.
  */
typedef enum kernel_request_type 
{
   NONE = 0,
   CREATE,
   NEXT,
   TERMINATE
} KERNEL_REQUEST_TYPE;

typedef void (*voidfuncptr) (void);      /* pointer to void f(void) */ 

/*********************/

/****MACROS***********/

#define BIT_RESET(PORT, PIN) (PORT &= ~(1<<PIN))
#define BIT_SET(PORT, PIN)   (PORT |= (1<<PIN))
#define BIT_TOGGLE(PORT, PIN) (PORT ^= (1<<PIN))
#define LOW_BYTE(X) (((uint16_t)X) & 0xFF)
#define HIGH_BYTE(X) ((((uint16_t)X) >> 8) & 0xFF)

/********************/

/****DEFINES***********/

#define MAXTHREAD     16       
#define WORKSPACE     256   // in bytes, per THREAD
#define MSECPERTICK   10   // resolution of a system TICK in milliseconds

/**********************/

#endif
