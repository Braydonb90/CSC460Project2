/**************************
 * This file holds the user "main" function, as well as the test tasks
 *************************/

#include "os.h"

#define TEST_SYSTEM
#define TEST_RR
//#define TEST_PID_CREATE


/*============
 * A Simple Test 
 *============
 */

/*
 * A cooperative "Ping" task.
 */
void Task_Ping_RR() 
{
    int  x ;
    BIT_SET(DDRA, 6);
    for(;;){
        //LED off
        BIT_SET(PORTA, 6);
        _delay_ms(400);

        BIT_RESET(PORTA, 6);
        _delay_ms(400);
    }
}


/*
 * A cooperative "Pong" task.
 */
void Task_Pong_RR() 
{
    int  x;
    BIT_SET(DDRB, 6);
    for(;;) {
        //LED on
        BIT_SET(PORTB, 6);
        _delay_ms(200);

        BIT_RESET(PORTB, 6);
        _delay_ms(200);
    }
}
/*
 * System version of "Ping" task.
 * Yields every iteration, terminates after arg iteration
 */
void Task_Ping_System() 
{
    int  arg = Task_GetArg();
    int i;
    BIT_SET(DDRA, 6);
    if(arg <= 0) {
        for(;;) {
            //LED on
            BIT_SET(PORTB, 6);
            _delay_ms(400);

            BIT_RESET(PORTB, 6);
            _delay_ms(400);

            Task_Next();
        }
    }
    else {
        for(i = 0; i<arg; i++){
            //LED off
            BIT_SET(PORTA, 6);
            _delay_ms(400);

            BIT_RESET(PORTA, 6);
            _delay_ms(400);

            Task_Next();
        }
    }
}


/*
 * System version of "Pong" task.
 * Yields every iteration, terminates after arg iterations if arg>=0
 */
void Task_Pong_System() 
{
    int  arg = Task_GetArg();
    int i;
    BIT_SET(DDRB, 6);
    if(arg <= 0) {
        for(;;) {
            //LED on
            BIT_SET(PORTB, 6);
            _delay_ms(200);

            BIT_RESET(PORTB, 6);
            _delay_ms(200);

            Task_Next();
        }
    }
    else {
        for(i = 0; i < arg; i++) {
            //LED on
            BIT_SET(PORTB, 6);
            _delay_ms(200);

            BIT_RESET(PORTB, 6);
            _delay_ms(200);

            Task_Next();
        }
    }
}


void user_main() {
#ifdef TEST_RR
    int p1 = Task_Create_RR(Task_Ping_RR, 0);
    int p2 = Task_Create_RR(Task_Pong_RR, 0); 
#endif
#ifdef TEST_SYSTEM
    int p3 = Task_Create_System(Task_Ping_System, 8);
    int p4 = Task_Create_System(Task_Pong_System, 3);
#endif
#if defined TEST_SYSTEM && defined TEST_RR && defined TEST_PID_CREATE
    debug_break(4, p1,p2,p3,p4);
#endif
}

