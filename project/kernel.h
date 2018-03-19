#ifndef KERNEL_H
#define KERNEL_H

#include "common.h"
#include "output.h"
#include "process_queue.h"
#include "os.h"

TICK Kernel_GetElapsed();

/*
 * external "main" function. first task to run, and should initialize the starting tasks
 */
extern void user_main();


/*===========
  * RTOS Internal
  *===========
  */

/**
  * This internal kernel function is the context switching mechanism.
  * It is done in a "funny" way in that it consists two halves: the top half
  * is called "Exit_Kernel()", and the bottom half is called "Enter_Kernel()".
  * When kernel calls this function, it starts the top half (i.e., exit). Right in
  * the middle, "Cp" is activated; as a result, Cp is running and the kernel is
  * suspended in the middle of this function. When Cp makes a system call,
  * it enters the kernel via the Enter_Kernel() software interrupt into
  * the middle of this function, where the kernel was suspended.
  * After executing the bottom half, the context of Cp is saved and the context
  * of the kernel is restore. Hence, when this function returns, kernel is active
  * again, but Cp is not running any more. 
  * (See file "switch.S" for details.)
  */
extern void CSwitch();
extern void Exit_Kernel();    /* this is the same as CSwitch() */

/* 
 * This external function could be implemented in two ways:
 *  1) as an external function call, which is called by Kernel API call stubs;
 *  2) as an inline macro which maps the call into a "software interrupt";
 *       as for the AVR processor, we could use the external interrupt feature,
 *       i.e., INT0 pin.
 *  Note: Interrupts are assumed to be disabled upon calling Enter_Kernel().
 *     This is the case if it is implemented by software interrupt. However,
 *     as an external function call, it must be done explicitly. When Enter_Kernel()
 *     returns, then interrupts will be re-enabled by Enter_Kernel().
 */ 
extern void Enter_Kernel();

/*
 * This is how the rest of the OS submits requests to the kernel
 *
 * The only "public" function
 * Param struct should be small enough that passing copies is fine, I think
 * Was concerned about structs going out of scope. Maybe declaring them as static would avoid that???
 */
void Kernel_Request();

#endif
