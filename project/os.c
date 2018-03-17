#include "os.h"

void OS_Abort(unsigned int error) {
    BIT_SET(DDRB, ERROR_PIN);
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

PID   Task_Create_System(voidfuncptr f, int arg) {
    //TODO
}
PID   Task_Create_RR(voidfuncptr f, int arg) {
    //TODO
    KERNEL_REQUEST_PARAM prm;
    prm.request_type = CREATE;
    prm.priority = RR;
    prm.code = f;
    prm.arg = arg;

    Kernel_Request(prm);
    return prm.pid;
}

/*
 * returns 0 if not successful; otherwise a non-zero PID.
 */
PID   Task_Create_Period(voidfuncptr f, int arg, TICK period, TICK wcet, TICK offset) {
    //TODO
}

/*
 * periodic task volunarily gives up cpu
 */   
void Task_Next() 
{
    KERNEL_REQUEST_PARAM prm;
    prm.request_type = NEXT;
    Kernel_Request(prm);
}

void Task_Terminate(void) {
    KERNEL_REQUEST_PARAM prm;
    prm.request_type = TERMINATE;
    Kernel_Request(prm);
}

/*
 * The calling task gets its initial "argument" when it was created.
 */
int  Task_GetArg(void) {
    //TODO
}


/*
 * returns the calling task's PID.
 */
PID  Task_Pid(void) {
    //TODO
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
    //TODO
}
PID  Msg_Recv( MASK m,           unsigned int *v )
{
    //TODO
}
void Msg_Rply( PID  id,          unsigned int r )
{
    //TODO
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
