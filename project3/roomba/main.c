/**************************
 * This file holds the user "main" function, as well as the test tasks
 *************************/

#include <util/delay.h>
#include "../os/common.h"
#include "../os/os.h"
#include "roomba.h"



roomba_sensor_data_t external;
roomba_sensor_data_t chassis;
roomba_sensor_data_t internal;

uint8_t is_escaping = 0;

void Roomba_UpdateSensorPacket_External() PERIODIC_TASK(
{
	Roomba_UpdateSensorPacket(EXTERNAL, &external);
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
	Roomba_ChangeDriveState();
	Roomba_Drive(250, 0);
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
	if(Roomba_BumperActivated(&external) || Roomba_RiverHit(&external))
	{
		if(is_escaping == 0) {
			is_escaping = 1;
			Task_Create_System(Roomba_Escape, 0);
		}
	}
}
)

void setup_tasks()
{
	Roomba_Init();
	Task_Create_Period(Roomba_ChangeMoveState, 0, 600, 5, 0);
	Task_Create_Period(Roomba_UpdateSensorPacket_External, 0, 15, 10, 10);
	//Task_Create_Period(Roomba_UpdateSensorPacket_Internal, 0, 25, 7, 550);
	Task_Create_Period(Roomba_CheckEnvironment, 0, 15, 10, 20);
	
}

void user_main() {
	printf("user_main\n");
	Task_Create_System(setup_tasks, 0);
}

