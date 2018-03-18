/**************************
 * This file holds the user "main" function, as well as the test tasks
 *************************/

#include "os.h"

#define TEST_SYSTEM

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
 * Yields every iteration
 */
void Task_Ping_System() 
{
    int  x ;
    BIT_SET(DDRA, 6);
    for(;;){
        //LED off
        BIT_SET(PORTA, 6);
        _delay_ms(400);

        BIT_RESET(PORTA, 6);
        _delay_ms(400);
          
        Task_Next();
    }
}


/*
 * System version of "Pong" task.
 * Yields every iteration
 */
void Task_Pong_System() 
{
    int  x;
    BIT_SET(DDRB, 6);
    for(;;) {
        //LED on
        BIT_SET(PORTB, 6);
        _delay_ms(200);

        BIT_RESET(PORTB, 6);
        _delay_ms(200);

        Task_Next();
    }
}


void user_main() {
#ifdef TEST_RR
    Task_Create_RR(Task_Ping_RR, 0);
    Task_Create_RR(Task_Pong_RR, 0); 
#endif
#ifdef TEST_SYSTEM
    Task_Create_System(Task_Ping_System, 0);
    Task_Create_System(Task_Pong_System, 0);
#endif
}

