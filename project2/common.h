#ifndef COMMON_H
#define COMMON_H

#include "uart.h"
#include <stdio.h>
#include <util/delay.h>
#include <avr/io.h>


/****TYPEDEFS*********/
typedef void (*voidfuncptr) (void);      /* pointer to void f(void) */ 
typedef unsigned int PID;        // always non-zero if it is valid
typedef unsigned int TICK;       // 1 TICK is defined by MSECPERTICK
typedef unsigned int BOOL;       // TRUE or FALSE
typedef unsigned char MTYPE;
typedef unsigned char MASK; 

/*
 * This is the set of priorities that a task can be scheduled with
 */
typedef enum priority {
    SYSTEM = 0,
    PERIODIC,
    RR,
    IDLE
} PRIORITY;

/*
 * This is the set of states that a task can be in at any given time.
 */
typedef enum process_state 
{ 
    DEAD = 0, 
    READY, 
    RUNNING,
	BLOCKED_SEND,
	BLOCKED_RECEIVE,
	BLOCKED_REPLY
} PROCESS_STATE;

/*
 * Set of possible errors. These are sent to OS_Abort
 */
typedef enum error_code {
    INVALID_REQUEST = 1,
    INVALID_PRIORITY_CREATE = 2,
    INVALID_PRIORITY_DISPATCH = 3,
    INVALID_STATE_DISPATCH = 4,
    INVALID_TERMINATE = 5,  
    NULL_REQUEST = 6,
    NON_NULL_REQUEST = 7,
    QUEUE_ERROR = 8,
    NO_DEAD_PDS = 9,        
    PERIODIC_OVERUSE = 10,
	TIMING_VIOLATION = 11,
    NON_NULL_Q_BACK = 12,
	INVALID_MSG_SEND_REQUEST = 13,
	INVALID_MSG_RECEIVE_REQUEST = 14,
	INVALID_MSG_REPLY_REQUEST = 15,
    DEBUG_IDLE_HALT = 16
} ERROR_CODE;    
typedef enum message_type
{
	MSG_TEST = 1,
	MSG_SYSTEM
} MESSAGE_TYPE;

typedef struct message
{
	PID pid;
	unsigned int *msg;
	unsigned int r;
	MTYPE type;
	MASK mask;
} MESSAGE;

/*
 * This is the set of kernel requests, i.e., a request code for each system call.
 */
typedef enum kernel_request_type 
{
    NONE = 0,
    CREATE,
    NEXT,
    TIMER_TICK,
    TERMINATE,
	SEND,
	RECEIVE,
	REPLY
} KERNEL_REQUEST_TYPE;

/* 
 * to pass info between kernel and tasks 
 */
typedef struct kernel_request_param 
{
    PID pid;                            //PID returned by kernel on creation etc.
    KERNEL_REQUEST_TYPE request_type;   //type of request
    voidfuncptr code;                    //code associated with process
    TICK wcet;
    TICK period;
    TICK offset;
    PRIORITY priority;
    int arg;    
	MESSAGE msg_detail;
} KERNEL_REQUEST_PARAM;

/*********************/

/****MACROS***********/

// I stole this. Much nicer than requiring periodic tasks to explicitly loop
#define PERIODIC_TASK(body)         \
    {                               \
        for(;; Task_Next()){        \
            body                    \
        }                           \
    }                               \


#define BIT_RESET(PORT, PIN) (PORT &= ~(1<<PIN))
#define BIT_SET(PORT, PIN)   (PORT |= (1<<PIN))
#define BIT_TOGGLE(PORT, PIN) (PORT ^= (1<<PIN))
#define LOW_BYTE(X) (((uint16_t)X) & 0xFF)
#define HIGH_BYTE(X) ((((uint16_t)X) >> 8) & 0xFF)
#define Disable_Interrupt()		asm volatile ("cli"::)
#define Enable_Interrupt()		asm volatile ("sei"::)

/********************/

/****DEFINES***********/

#define MAXTHREAD     16       
#define WORKSPACE     256   // in bytes, per THREAD
#define MSECPERTICK   10   // resolution of a system TICK in milliseconds
#define BLINKDELAY 200

//These pins are on port B
#define OUTPUT_PORT_INIT DDRB
#define OUTPUT_PORT PORTB

#define ERROR_PIN 7
#define IDLE_PIN 6
#define CLOCK_PIN 5
#define DEBUG_PIN 4

//System Tasks on Port A ==> (Pin22 == PA0) to (Pin29 == PA7)
#define SYSTEM_PORT_INIT DDRA
#define SYSTEM_PORT PORTA

//Periodic Tasks on Port C ==> (Pin37 == PC0) to (Pin30 == PC7)
#define PERIODIC_PORT_INIT DDRC
#define PERIODIC_PORT PORTC

//Round Robin Tasks on Port L ==> (Pin49 == PL0) to (Pin42 == PL7)
#define RR_PORT_INIT DDRL
#define RR_PORT PORTL

#ifndef NULL
#define NULL          0   /* undefined */
#endif
#define TRUE          1
#define FALSE         0

#define DEBUG         0

#define ANY           0xFF       // a mask for ALL message type

/**********************/
extern BOOL do_break;

#endif
