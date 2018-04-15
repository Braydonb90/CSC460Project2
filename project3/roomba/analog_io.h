#include "../os/common.h"

#define TILTMIN 250
#define TILTMAX 550
#define PANMIN 250
#define PANMAX 550
#define MAXDIFF 25
#define DEADBANDMIN 108
#define DEADBANDMAX 148

int pos_x;
int pos_y;

// Initialize ADC. Used to get analog values from pins (i.e joystick input)
void analog_init();

// Read analog value at pin
uint8_t analog_read(uint8_t pin);

void test_servo();
void servo_init(); 

void servo_set_laser(uint8_t val);
void servo_set_pan(uint8_t pos);
void servo_set_tilt(uint8_t pos);

long map(long x, long in_min, long in_max, long out_min, long out_max);