#include "../os/os.h" 
#include "../os/common.h"
#include "analog_io.h"
#include <util/delay.h>

#define XPIN 0
#define YPIN 1

void query_analog() {
    for(;;) {
        _delay_ms(500);
        printf("X is %d\n", analog_read(XPIN));
        printf("Y is %d\n", analog_read(YPIN));
    }
}

void user_main() {
   Task_Create_System(analog_init, 0);     
   Task_Create_RR(query_analog, 0);
}
