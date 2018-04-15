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
}

int pos_x[2];
int pos_y[2];
uint8_t laser_state;

void joystick_init() {
    //Set Z pins to pull-up input
    BIT_RESET(DDRA, PINZ0);
    BIT_RESET(DDRA, PINZ1);
    BIT_SET(PORTA, PINZ0);
    BIT_SET(PORTA, PINZ1);

    pos_x[0] = 128;
    pos_y[0] = 128;
    laser_state = 0;
    pos_x[1] = 128;
    pos_y[1] = 128;
}

static long map(long x, long in_min, long in_max, long out_min, long out_max)
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
uint8_t query_joystick_x(int num) {
    int val;
    if(num == 0) {
        val = apply_deadband(analog_read(PINX0));
    }
    else {
        val = apply_deadband(analog_read(PINX1));
    }
    pos_x[num] = val;
    return val;
}
uint8_t query_joystick_y(int num) {
    int val;
    if(num == 0) {


    }
    else {
        val = apply_deadband(analog_read(PINY1));
    }
    pos_y[num] = val;
    return val;
}
int released = 1;
uint8_t query_joystick_z(int num) {
    if(num == 0) {
        uint8_t val = BIT_READ(PINA, PINZ0);
        if(val == 0 && released == 1) {
            released = 0;
            laser_state = !laser_state;
        }
        else if(released == 0  && val == 1) {
            released = 1;
        }
        return val;
    }
    else {  //prob not used???
        return BIT_READ(PINA, PINZ1);
    }
}
uint8_t get_laserstate() {
    return laser_state;
}

//Pin2
void servo_set_pan(uint8_t pos) {
    pos = apply_deadband(pos);
    int diff = map(pos, 0, 255, -MAXDIFF, MAXDIFF);
    pos_x[0] += diff;
    pos_x[0] = constrain(pos_x[0], PANMIN,PANMAX);
    OCR3B = pos_x[0];
}

//Pin3
void servo_set_tilt(uint8_t pos) {
    pos = apply_deadband(pos);
    int diff = map(pos, 0, 255, -MAXDIFF, MAXDIFF);
    pos_y[0] += diff;
    pos_y[0] = constrain(pos_y[0], TILTMIN,TILTMAX);
    OCR3C = pos_y[0];
}

//Should maybe add marker values for start and end
//Packet Contents:
//  Byte0:  pos_x[0] (joystick0 pos)
//  Byte1:  pos_y[0] (joystick0 pos)
//  Byte2:  laser state 
//  Byte3:  pos_x[1] (joystick1 pos)
//  Byte4:  pos_y[1] (joystick1 pos)

void send_packet() {
    int packet_size = 6;
    uint8_t buff[packet_size];
    
    buff[0] = 255;
    buff[1] = constrain(pos_x[0], 0, 254);
    buff[2] = constrain(pos_y[0], 0, 254);
    buff[3] = laser_state;
    buff[4] = constrain(pos_x[1], 0, 254);
    buff[5] = constrain(pos_y[1], 0, 254);
    printf("-------------\n1 %d\n2 %d\n3 %d\n4 %d\n5 %d\n-------------\n",buff[1],buff[2],buff[3],buff[4], buff[5]);
    uart1_print(buff, packet_size);
}

//Unpack packet values
void update_values(uint8_t* packet) {
    servo_set_pan(packet[0]);
    servo_set_tilt(packet[1]);
    laser_state = packet[2];
}

