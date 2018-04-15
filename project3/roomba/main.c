/**************************
 * This file holds the user "main" function, as well as the test tasks
 *************************/

#include <util/delay.h>
#include "../os/common.h"
#include "../os/os.h"
#include "roomba.h"
#include "analog_io.h"

uint8_t ambient_light;
#define MAX_LIGHT_DIFF 20

roomba_sensor_data_t external;
roomba_sensor_data_t chassis;
roomba_sensor_data_t internal;

uint8_t is_escaping = 0;

uint8_t packet[6];
uint8_t packet_index = 0;

extern int pos_x;
extern int pos_y;

void Roomba_UpdateSensorPacket_External() PERIODIC_TASK(
{
	BIT_SET(PORTA, 0);
	Roomba_UpdateSensorPacket(EXTERNAL, &external);
	BIT_RESET(PORTA, 0);
}
)

void Roomba_UpdateSensorPacket_Chassis() PERIODIC_TASK(
{
	Roomba_UpdateSensorPacket(CHASSIS, &chassis);
}
)

void Roomba_UpdateSensorPacket_Internal() PERIODIC_TASK(
{
	Roomba_UpdateSensorPacket(INTERNAL, &internal);
}
)

void Roomba_ChangeMoveState() PERIODIC_TASK(
{
	BIT_SET(PORTA, 1);
	Roomba_ChangeDriveState();
	BIT_RESET(PORTA, 1);
}
)

void Kill() 
{
	for(;;) {
		Roomba_ConfigPowerLED(POWER_RED, 255);
		_delay_ms(500);
		Roomba_ConfigPowerLED(POWER_RED, 0);
		_delay_ms(500);
	}
}

void Query_LightSensor() PERIODIC_TASK(
{
	uint8_t val = analog_read(1);
	//printf("%d\n", val);
	if(val >= ambient_light*1.5)
	{
		Task_Create_System(Kill, 0);
	}
}
)

void Roomba_Escape()
{
	Roomba_ConfigPowerLED(POWER_GREEN, 255);
		
		//Roomba_UpdateSensorPacket(CHASSIS, &chassis);
		Roomba_Drive(-200, 0);		// reverse direction
  		//int16_t total_distance = 0;		// reset the total distance; going backwards ==> negative distance
  		/*while (total_distance > -150)
  		{
  			total_distance += chassis.distance.value;
  			_delay_ms(10);
			Roomba_UpdateSensorPacket(CHASSIS, &chassis);
  		}*/
		_delay_ms(250);
  		Roomba_Drive(0, 0);
		
	Roomba_ConfigPowerLED(POWER_RED, 255);
	is_escaping = 0;
}

void Roomba_CheckEnvironment() PERIODIC_TASK(
{
	BIT_SET(PORTA, 3);
	if(Roomba_BumperActivated(&external) || Roomba_RiverHit(&external))
	{
		if(is_escaping == 0) {
			is_escaping = 1;
			Task_Create_System(Roomba_Escape, 0);
		}
	}
	BIT_RESET(PORTA, 3);
}
)

void Setup_Ambient_Light() {
	ambient_light = 0;
	_delay_ms(500);
	int i;
	for(i = 0; i < 10; i++)
	{
		ambient_light += analog_read(1);
		_delay_ms(100);
	}
	ambient_light = ambient_light/10;
	
	printf("Light: %d\n", ambient_light);
}

void Read_Bluetooth() PERIODIC_TASK(
{
	uint8_t num_bytes = uart1_bytes_received();
	int i;
	for(i = 0; i < num_bytes; i++) {
		if(uart1_get_byte(i) == 255)
		{
			packet_index = 0;
		}
		if(packet_index >= 6) break;
		packet[packet_index] = uart1_get_byte(i);
		packet_index++;
	}
	
	uart1_reset_receive();
}
)

void Set_Servo() PERIODIC_TASK(
{
	servo_set_pan(packet[1]);
	servo_set_tilt(packet[2]);
	servo_set_laser(packet[3]);
}
)

int abs(int v) {
	if(v < 0) return -v;
	return v;
}

void Set_Roomba() PERIODIC_TASK(
{
	int vel = -map(packet[5], 0, 255, -350, 350);
	int rad = map(packet[4], 0, 255, -2000, 2000);
	if(abs(vel) < 100) {
		vel = 0;
		
		if(rad > 10) {
			Roomba_D_Drive(100, -100);
		} else if(rad < -10) {
			Roomba_D_Drive(-100, 100);
		} else {
			Roomba_Drive(0, 0);
		}
	} else if(abs(rad) < 400) {
		rad = 0x7FFF;
		Roomba_Drive(vel, rad);
	}
}
)

void setup_tasks()
{
	Roomba_Init();
	analog_init();
	servo_init();
	uart1_init(BAUD_CALC(9600));
	Setup_Ambient_Light();
	
	Task_Create_Period(Roomba_ChangeMoveState, 0, 6000, 2, 0);  // 0.5ms execution time
	Task_Create_Period(Roomba_UpdateSensorPacket_External, 0, 25, 5, 5); // 14.5ms execution time*/
	//Task_Create_Period(Roomba_UpdateSensorPacket_Internal, 0, 25, 7, 550); // 0.55ms execution time
	Task_Create_Period(Roomba_CheckEnvironment, 0, 25, 2, 10); // 0.27ms execution time
	Task_Create_Period(Query_LightSensor, 0, 50, 2, 13); // 2.9us execution time
	Task_Create_Period(Read_Bluetooth, 0, 25, 2, 16); // 0.6ms execution time
	Task_Create_Period(Set_Roomba, 0, 25, 5, 20); // 4ms execution time
	Task_Create_Period(Set_Servo, 0, 25, 2, 23); // 2.6us execution time
	
}

void user_main() {
	Task_Create_System(setup_tasks, 0);
}

