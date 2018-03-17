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
