/**************************
 * This file holds the user "main" function, as well as the test tasks
 *************************/

#include <util/delay.h>
#include "common.h"
#include "os.h"

//delay function needs constants :(
//  these are the prescribed periodic task execution times in ticks
#define PERIODIC_PING_ET 50
#define PERIODIC_PONG_ET 50

#define TEST_SYSTEM
#define TEST_RR
//#define TEST_PERIODIC
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
    BIT_SET(RR_PORT_INIT, 0);
    for(;;){
        //LED off
        BIT_SET(RR_PORT, 0);
        _delay_ms(400);

        BIT_RESET(RR_PORT, 0);
        _delay_ms(400);
    }
}


/*
 * A cooperative "Pong" task.
 */
void Task_Pong_RR() 
{
    int  x;
    BIT_SET(RR_PORT_INIT, 1);
    for(;;) {
        //LED on
        BIT_SET(RR_PORT, 1);
        _delay_ms(200);

        BIT_RESET(RR_PORT, 1);
        _delay_ms(200);
    }
}


/*
 * A cooperative "Ping" task.
 */
void Task_Ping_Periodic() PERIODIC_TASK( 
{
    BIT_SET(PERIODIC_PORT_INIT, 0);
    BIT_TOGGLE(PERIODIC_PORT, 0);
}
)


/*
 * A cooperative "Pong" task.
 */
void Task_Pong_Periodic() PERIODIC_TASK(
{
    BIT_SET(PERIODIC_PORT_INIT, 1);
    BIT_TOGGLE(PERIODIC_PORT, 1);
}
)
/*
 * System version of "Ping" task.
 * Yields every iteration, terminates after arg iteration
 */
void Task_Ping_System() 
{
    int  arg = Task_GetArg();
    int i;
    BIT_SET(SYSTEM_PORT_INIT, 0);
    if(arg <= 0) {
        for(;;) {
            BIT_SET(SYSTEM_PORT, 0);
            _delay_ms(400);

            BIT_RESET(SYSTEM_PORT, 0);
            _delay_ms(400);

            Task_Next();
        }
    }
    else {
        for(i = 0; i<arg; i++){
            BIT_SET(SYSTEM_PORT, 0);
            _delay_ms(400);

            BIT_RESET(SYSTEM_PORT, 0);
            _delay_ms(400);

            Task_Next();
        }
    }
#ifdef TEST_PERIODIC
    Task_Create_Period(Task_Ping_Periodic, 0, 400, 20, 0); // arg period wcet offset
    Task_Create_Period(Task_Pong_Periodic, 0, 400, 20, 50);
#endif
}


/*
 * System version of "Pong" task.
 * Yields every iteration, terminates after arg iterations if arg>0
 */
void Task_Pong_System() 
{
    int  arg = Task_GetArg();
    int i;
    BIT_SET(SYSTEM_PORT_INIT, 1);
    if(arg <= 0) {
        for(;;) {
            //LED on
            BIT_SET(SYSTEM_PORT, 1);
            _delay_ms(200);

            BIT_RESET(SYSTEM_PORT, 1);
            _delay_ms(200);

            Task_Next();
        }
    }
    else {
        for(i = 0; i < arg; i++) {
            //LED on
            BIT_SET(SYSTEM_PORT, 1);
            _delay_ms(200);

            BIT_RESET(SYSTEM_PORT, 1);
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
int start_p5 = 0;
int start_p6 = 1;
#ifdef TEST_SYSTEM
    int arg_p3 = 8;
    int arg_p4 = 5;
    int ex_time = arg_p3*800/MSECPERTICK + arg_p4*400/MSECPERTICK;
    int p3 = Task_Create_System(Task_Ping_System, arg_p3);
    int p4 = Task_Create_System(Task_Pong_System, arg_p4);
#elif defined TEST_PERIODIC 
    int p5 = Task_Create_Period(Task_Ping_Periodic, 0, 400, 20, start_p5); // arg period wcet offset
    int p6 = Task_Create_Period(Task_Pong_Periodic, 0, 400, 20, start_p6);
#endif
#if defined TEST_SYSTEM && defined TEST_RR && defined TEST_PID_CREATE
    //debug_break(4, p1,p2,p3,p4);
#endif
}

