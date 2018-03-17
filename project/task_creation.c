/**************************
 * This file holds the user "main" function, as well as the test tasks
 *************************/

#include "os.h"

/*============
 * A Simple Test 
 *============
 */

/*
 * A cooperative "Ping" task.
 */
void Task_Ping() 
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
 * A cooperative "Pong" task.
 */
void Task_Pong() 
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
   Task_Create_RR(Task_Ping, 0);
   Task_Create_RR(Task_Pong, 0); 
}

