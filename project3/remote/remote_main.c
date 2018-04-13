#include "../os/os.h" 
#include "../os/common.h"
#include "analog_io.h"
#include <util/delay.h>

#define XPIN 0
#define YPIN 1

void query_joystick() {
    int since_last_print = 0;
    for(;;) {
        int x = query_joystick_x(0);
        int y = query_joystick_y(0);
        int z = query_joystick_z(0);
        if(since_last_print > 500) {
            printf("X is %d\n", x);
            printf("Y is %d\n", y);
            printf("Z is %d\n", z);
            since_last_print = 0;
        }
        else {
            since_last_print += 20;
        }
        servo_set_pan(x);
        servo_set_tilt(y);
        _delay_ms(20);

    }
}

void user_main() {
    Task_Create_System(analog_init, 0);     
    Task_Create_System(servo_init, 0);
    Task_Create_System(joystick_init, 0);
    Task_Create_RR(query_joystick, 0);
}

