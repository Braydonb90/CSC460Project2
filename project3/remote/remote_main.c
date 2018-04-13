#include "../os/os.h" 
#include "../os/common.h"
#include "../os/uart.h"
#include "analog_io.h"
#include <util/delay.h>

#define XPIN 0
#define YPIN 1

void query_joystick() {
    int since_last_print = 0;
    int i;
    int buff_size = 20;
    char buffer[20];
    uart1_init(9600);
    buffer[19] = '\0';
    int buff_index = 0;
    for(;;) {
        int x = query_joystick_x(0);
        int y = query_joystick_y(0);
        int z = query_joystick_z(0);
        if(since_last_print > 500) {
            printf("X is %d\n", x);
            printf("Y is %d\n", y);
            printf("Z is %d\n", z);
            since_last_print = 0;
            buffer[buff_index] = '\0';
            printf("received %d bytes:\n\t%s\n------\n",buff_index, buffer);
            uart1_reset_receive();
            buff_index = 0;
        }
        else {
            since_last_print += 20;
        }
        int received = uart1_bytes_received();
        if(received > 0) {
            int index = 0;
            for(i=buff_index; i<buff_size-1 && index<received; i++) {
                buffer[i] = uart1_get_byte(index);
                index++;
            }
            buff_index = i;
        }
        fprintf(&uart1_output, "test\n");
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

