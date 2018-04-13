#include "../os/os.h" 
#include "../os/common.h"
#include "../os/uart.h"
#include "analog_io.h"
#include <util/delay.h>

#define XPIN 0
#define YPIN 1

void query_joystick() {
    int since_last_send = 0;
    int i;
    int buff_size = 20;
    char buffer[20];

    //shouldnt this macro be called in uart1_init?
    uart1_init(BAUD_CALC(9600));
    for(;;) {
        query_joystick_x(0);
        query_joystick_y(0);
        query_joystick_z(0);
        
        if(since_last_send > 500) {
            puts("sending packet");
            send_packet();
            since_last_send = 0;
        }
        else {
            since_last_send += 20;
        }
        _delay_ms(20);
    }
}

void user_main() {
    Task_Create_System(analog_init, 0);     
    Task_Create_System(servo_init, 0);
    Task_Create_System(joystick_init, 0);
    Task_Create_RR(query_joystick, 0);
}
