#include "../os/os.h" 
#include "../os/common.h"
#include "../os/uart.h"
#include "analog_io.h"
#include <util/delay.h>

void communication_init() {
    uart1_init(BAUD_CALC(9600));
}
void send_data() PERIODIC_TASK(
{
    send_packet();
}
)

void query_joystick() {
    //shouldnt this macro be called in uart1_init?
    uart1_init(BAUD_CALC(9600));
    for(;;) {
        query_joystick_x(0);
        query_joystick_y(0);
        query_joystick_z(0);
        query_joystick_x(1);
        query_joystick_y(1);
        _delay_ms(20);
    }
}

void user_main() {
    Task_Create_System(analog_init, 0);     
    Task_Create_System(servo_init, 0);
    Task_Create_System(joystick_init, 0);
    Task_Create_System(communication_init, 0);
    Task_Create_Periodic(send_packet, 0, 10, 5, 0);
    Task_Create_RR(query_joystick, 0);
}
