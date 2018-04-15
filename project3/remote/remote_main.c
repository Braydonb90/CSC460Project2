#include "../os/os.h" 
#include "../os/common.h"
#include "../os/uart.h"
#include "analog_io.h"
#include <util/delay.h>

void query_joystick() {
    int since_last_print = 0;
    int since_last_send = 0;
    int i;
    int buff_size = 20;
    char buffer[20];

    //shouldnt this macro be called in uart1_init?
    uart1_init(BAUD_CALC(9600));
    for(;;) {
        if(since_last_print > 500) {
            int x0 = query_joystick_x(0);
            int y0 = query_joystick_y(0);
            int z0 = query_joystick_z(0);
            int x1 = query_joystick_x(1);
            int y1 = query_joystick_y(1);
            int z1 = query_joystick_z(1);
            int ls = get_laserstate();
            since_last_print = 0;
        }
        else {
            since_last_print += 20;
        }
        if(since_last_send > 250) {
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
