#include <avr/io.h>
#include "../os/common.h"

#define TILTMIN 250
#define TILTMAX 550
#define PANMIN 250
#define PANMAX 550
#define MAXDIFF 5
#define DEADBANDMIN 108
#define DEADBANDMAX 148

#define PINX0 0
#define PINY0 1
#define PINZ0 0 //(PA0 == Pin22)
#define PINX1 2
#define PINY1 3
#define PINZ1 1 //(PA1 == Pin23)


// Initialize ADC. Used to get analog values from pins (i.e joystick input)
void analog_init();

// Read analog value at pin
uint8_t analog_read(uint8_t pin);
void test_servo();
void servo_init(); 
void joystick_init();
uint8_t query_joystick_x(int num);
uint8_t query_joystick_y(int num);
uint8_t query_joystick_z(int num);
uint8_t get_laserstate();
void servo_set_pan(uint8_t pos);
void servo_set_tilt(uint8_t pos);
