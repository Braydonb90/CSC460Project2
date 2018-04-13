/**************************
 * This file holds the user "main" function, as well as the test tasks
 *************************/

#include <util/delay.h>
#include "../os/common.h"
#include "../os/os.h"
#include "roomba.h"



void user_main() {
	Task_Create_System(Roomba_Init, 0);
}

