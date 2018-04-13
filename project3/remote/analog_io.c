#include "analog_io.h"


// Initialize ADC. Used to get analog values from pins (i.e joystick input)
void analog_init() {
    // Set prescaler to 128 (125kHz)
    BIT_SET(ADCSRA, ADPS2);
    BIT_SET(ADCSRA, ADPS1);
    BIT_SET(ADCSRA, ADPS0);

    // Set ADC reference voltage
    BIT_SET(ADMUX, REFS0);

    // Left align ADC value, so that entire value is contained in ADCH
    BIT_SET(ADMUX, ADLAR);

    // Enable ADC
    BIT_SET(ADCSRA, ADEN);
    
    // Start Conversion
    BIT_SET(ADCSRA, ADSC);

    // Use entire PORTC as analog input
    PORTC = 0xFF;
}

// Read analog value at pin
uint8_t analog_read(uint8_t pin) {
    // clear mux bits 0-4
    ADMUX &= 0xE0;
    
    // set channel (mask it so we dont overwrite shit)
    ADMUX |= (7 & pin);

    // start new conversion
    BIT_SET(ADCSRA, ADSC);
    
    while((ADCSRA >> ADSC) & 1);

    return ADCH;
}
