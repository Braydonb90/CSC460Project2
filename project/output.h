#ifndef OUTPUT_H
#define OUTPUT_H

#include "common.h"
#include <stdarg.h>

/*
 *This file is used to store any functions related to output from the board
 */

/*
 * Blink specified pin num times. Used for debugging and error codes
 * Currently assumes pin is on port B
 */
void Blink_Pin(unsigned int pin, unsigned int num);

/*
 * Basic debugging function to repeatedly blink LED corresponding to list of values
 * Useful for breaking execution and listing values
 */
void debug_break(int argcount, ...);

/*
 * Same as debug_break, but only goes through list once and then returns
 */
void debug_blink(int argcount, ...); 


#endif
