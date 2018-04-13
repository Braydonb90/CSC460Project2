#include <avr/io.h>
#include "../os/common.h"

// Initialize ADC. Used to get analog values from pins (i.e joystick input)
void analog_init();

// Read analog value at pin
uint8_t analog_read(uint8_t pin);
