#include "output.h"


void Blink_Pin(unsigned int pin, unsigned int num){
    int i;
    for(i = 0; i < num; i++){
        BIT_SET(PORTB, pin);
        _delay_ms(BLINKDELAY);
        BIT_RESET(PORTB, pin);
        _delay_ms(BLINKDELAY);
    }
}

void debug_break(int argcount, ...) {
    Disable_Interrupt();
    if(argcount > 20) {
        while(TRUE);
    }

    int buffer[20], i, num;
    va_list args;
    va_start(args, argcount);
    for(i = 0; i < argcount; i++) {
        buffer[i] = va_arg(args, int);
    }
    va_end(args);
    while(TRUE) {
        debug_blink_array(argcount, buffer);
        _delay_ms(1000);
    }
}
void debug_blink(int argcount, ...) {
    va_list args;
    va_start(args, argcount);
    int buffer[20];
    int i;

    for(i = 0; i<argcount; i++) {
        buffer[i] = va_arg(args, int);
    }
    debug_blink_array(argcount, buffer);
}
static void debug_blink_array(int count, int* arr) {
    int i, num;
    for(i = 0; i < count; i++) { 
        num = arr[i];
        _delay_ms(500);
        if(num > 10 ||  num < 0) {
            BIT_SET(PORTB, DEBUG_PIN);
            _delay_ms(500);
            BIT_RESET(PORTB, DEBUG_PIN);
        }
        else{
            Blink_Pin(DEBUG_PIN, num);
        }
    }
}

