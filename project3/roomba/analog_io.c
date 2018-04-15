#include "analog_io.h"

int laser_time = 0;
int last_laser = 0;

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

void servo_init() {
    BIT_SET(DDRE, 4); //Pin 2 (OC3B/PWM) as OUT
    BIT_SET(DDRE, 5); //Pin 3 (OC3C/PWM) as OUT

    //Set timer3 to fast PWM mode
    //This means that counter counts from BOTTOM to TOP. Once top is reached, 
    //OCR registers are updated and TCNT is reset to BOTTOM
    //BOTTOM is 0
    //TOP is whatever OCR3A is set to  
    BIT_SET(TCCR3A, WGM30);
    BIT_SET(TCCR3A, WGM31);
    BIT_SET(TCCR3B, WGM32);
    BIT_SET(TCCR3B, WGM33);

    BIT_SET(TCCR3A, COM3C1); //Clear OCR3C on compare match. Set OCR3C at bottom
    BIT_SET(TCCR3A, COM3B1); //Clear OCR3B on compare match. Set OCR3B at bottom

    //Clock select 3: Clock_IO / 64 = 250kHz
    BIT_SET(TCCR3B, CS31);
    BIT_SET(TCCR3B, CS30);

    //PWM 
    //Manual Page 147
    //OCR match when OCR val == CNT
    //The value of OCR3A is used as the period
    //Since COM3C1 and COM3B1 are set, each period starts with OCR3C/OCR3B's corresponding pins being high
    //Once OCR3C/OCR3B matches TCNT, the corresponding pin is set to low.
    OCR3A = 5000; //250000 / 5000 = 50Hz  --> 20 ms period
    OCR3B = (PANMAX-PANMIN)/2; //250000 / 375 =  666.6Hz  --> 1.5 ms
    OCR3C = (TILTMAX-TILTMIN)/2;
	
	BIT_SET(DDRB, 1);
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static int constrain(int in, int min, int max) {
    if(in < min) return min;
    if(in > max) return max;

    return in;
}
static int apply_deadband(int val) {
    if(val > DEADBANDMIN && val < DEADBANDMAX) {
        return 128;
    }
    else {
        return val;
    }
}


int	pos_x = (PANMAX - PANMIN) / 2;
int	pos_y = (TILTMAX - TILTMIN) / 2;

void servo_set_laser(uint8_t val)
{
	if(val == 1 && laser_time < 3000) {
		if(last_laser == 0) {
			last_laser = Kernel_GetElapsed();
		} else {
			int cur_laser = Kernel_GetElapsed();
			laser_time += cur_laser - last_laser;
			last_laser = cur_laser;
		}
		BIT_SET(PORTB, 1);
	} else {
		BIT_RESET(PORTB, 1);
	}
}



//Pin2
void servo_set_pan(uint8_t pos) {
    pos = apply_deadband(pos);
    int diff = map(pos, 0, 255, -MAXDIFF, MAXDIFF);
    pos_x += diff;
    pos_x = constrain(pos_x, PANMIN,PANMAX);
    OCR3B = pos_x;
}

//Pin3
void servo_set_tilt(uint8_t pos) {
    pos = apply_deadband(pos);
    int diff = map(pos, 0, 255, -MAXDIFF, MAXDIFF);
    pos_y += diff;
    pos_y = constrain(pos_y, TILTMIN,TILTMAX);
    OCR3C = pos_y;
}

