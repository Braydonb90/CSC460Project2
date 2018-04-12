#include "os.h"

void OS_Abort(unsigned int error) {
    Disable_Interrupt();
	switch(error) {
			case INVALID_REQUEST:
				printf("ERROR: INVALID_REQUEST\n");
				break;
			case INVALID_PRIORITY_CREATE:
				printf("ERROR: INVALID_PRIORITY_CREATE\n");
				break;
			case INVALID_PRIORITY_DISPATCH:
				printf("ERROR: INVALID_PRIORITY_DISPATCH\n");
				break;
			case INVALID_STATE_DISPATCH:
				printf("ERROR: INVALID_STATE_DISPATCH\n");
				break;
			case INVALID_TERMINATE:
				printf("ERROR: INVALID_TERMINATE\n");
				break;
			case DEBUG_IDLE_HALT:
				printf("ERROR: DEBUG_IDLE_HALT\n");
				break;
			case NULL_REQUEST:
				printf("ERROR: NULL_REQUEST\n");
				break;
			case NON_NULL_REQUEST:
				printf("ERROR: NON_NULL_REQUEST\n");
				break;
			case QUEUE_ERROR:
				printf("ERROR: QUEUE_ERROR\n");
				break;
			case NO_DEAD_PDS:
				printf("ERROR: NO_DEAD_PDS\n");
				break;
			case INVALID_MSG_SEND_REQUEST:
				printf("ERROR: INVALID_MSG_SEND_REQUEST\n");
				break;
			case INVALID_MSG_RECEIVE_REQUEST:
				printf("ERROR: INVALID_MSG_RECEIVE_REQUEST\n");
				break;
			case INVALID_MSG_REPLY_REQUEST:
				printf("ERROR: INVALID_MSG_REPLY_REQUEST\n");
				break;
			case TIMING_VIOLATION:
				printf("ERROR: TIMING_VIOLATION\n");
				break;
			default:
				printf("ERROR: %d\n", error);
				break;
		}
    while(TRUE) {
        Blink_Pin(ERROR_PIN, error);
        _delay_ms(1000); 
    }
}


void OS_Init(void) {
    
}

/*
 * Scheduling Policy:
 * There are three priority levels:
 *   HIGHEST  -- System tasks,
 *   MEDIUM   -- Periodic tasks,
 *   LOWEST   -- Round-Robin (RR) tasks
 */
 
PID Task_Create(voidfuncptr f, PRIORITY p, int arg, TICK period, TICK wcet, TICK offset) {
	KERNEL_REQUEST_PARAM prm;
    prm.request_type = CREATE;
    prm.priority = p;
    prm.code = f;
    prm.arg = arg;
	prm.period = period;
	prm.wcet = wcet;
	prm.offset = offset;

    Kernel_Request(&prm);
    return prm.pid;
}

PID   Task_Create_System(voidfuncptr f, int arg) {
    return Task_Create(f, SYSTEM, arg, 0, 0, 0);
}
PID   Task_Create_RR(voidfuncptr f, int arg) {
    return Task_Create(f, RR, arg, 0, 0, 0);
}

/*
 * returns 0 if not successful; otherwise a non-zero PID. -- //TODO PID 0 not valid?
 */
PID   Task_Create_Period(voidfuncptr f, int arg, TICK period, TICK wcet, TICK offset) {
    return Task_Create(f, PERIODIC, arg, period, wcet, offset);
}

/*
 * periodic task volunarily gives up cpu
 */   
void Task_Next() 
{
    KERNEL_REQUEST_PARAM prm;
    prm.request_type = NEXT;
    Kernel_Request(&prm);
}

void Task_Terminate(void) {
    KERNEL_REQUEST_PARAM prm;
    prm.request_type = TERMINATE;
    Kernel_Request(&prm);
}

/*
 * The calling task gets its initial "argument" when it was created.
 */
int  Task_GetArg(void) {
    return Kernel_GetArg();
}


/*
 * returns the calling task's PID.
 */
PID  Task_Pid(void) {
    return Kernel_GetPid();
}

unsigned int Now() {
	return Kernel_GetElapsed() * MSECPERTICK + TCNT4/62.5; //62.5 per millisecond
}


/*
 * Send-Recv-Rply is similar to QNX-style message-passing
 * Rply() to a NULL process is a no-op.
 * See: http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_sys_arch%2Fipc.html
 *
 * Note: PERIODIC tasks are not allowed to use Msg_Send() or Msg_Recv().
 */
void Msg_Send( PID  id, MTYPE t, unsigned int *v )
{
    KERNEL_REQUEST_PARAM prm;
    prm.request_type = SEND;
	prm.msg_detail.msg = v;
	prm.msg_detail.type = t;
	prm.msg_detail.pid = id;
    Kernel_Request(&prm);
}
PID  Msg_Recv( MASK m,           unsigned int *v )
{
	printf("Receive Mask: %d\n", m);
    KERNEL_REQUEST_PARAM prm;
    prm.request_type = RECEIVE;
	prm.msg_detail.mask = m;
	prm.msg_detail.msg = v;
    Kernel_Request(&prm);
	
	return prm.msg_detail.pid;
}
void Msg_Rply( PID  id,          unsigned int r )
{
    KERNEL_REQUEST_PARAM prm;
    prm.request_type = REPLY;
	prm.msg_detail.r = r;
	prm.msg_detail.pid = id;
    Kernel_Request(&prm);
}

/*
 * Asychronously Send a message "v" of type "t" to "id". The task "id" must be blocked on
 * Recv() state, otherwise it is a no-op. After passing "v" to "id", the returned PID of
 * Recv() is NULL (non-existent); thus, "id" doesn't need to reply to this message.
 * Note: The message type "t" must satisfy the MASK "m" imposed by "id". If not, then it
 * is a no-op.
 *
 * Note: PERIODIC tasks (or interrupt handlers), however, may use Msg_ASend()!!!
 */
void Msg_ASend( PID  id, MTYPE t, unsigned int v )
{
    //TODO
}
