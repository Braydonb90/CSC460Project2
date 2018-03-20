#include "process_queue.h"

static PD* Q_Pop_Internal(ProcessQ* q);

ProcessQ* Q_Init(ProcessQ* q, PRIORITY type){
    q->front = NULL;
    q->back = NULL;
    q->length = 0;
    q->queue_type = type;

    return q;
}

/*
 * for RR and system tasks
 */
void Q_Push(ProcessQ* q, PD* pd) {
    if (q->length == 0) {
        pd->next = NULL;
        q->front = pd;
        q->back = pd;
        q->length++;
    }
    else {
        q->back->next = pd;
        q->back = pd;
        q->length++;
    }
}
/*
 * Cycles through Q, returning the first READY task
 */
PD* Q_Pop_Ready(ProcessQ* q) {
    int len = q->length,i;
    PD* prev = q->front;
    PD* cur = prev->next;
    //in case the front is actually ready
    if(prev->state == READY){
        q->front = cur;
        q->length--;
        return prev;
    }
    //otherwise
    while(cur != NULL){
        if(cur->state == READY){
            prev->next = cur->next;
            cur->next = NULL;
            q->length--;
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
            
    return NULL;
}
/*
 * Regular pop
 */
PD* Q_Pop(ProcessQ* q) {
    if(q->front == NULL)
        return NULL;

    PD* ret = q->front;
    q->front = q->front->next;
    q->length--;
    return ret;
}

PD* Q_Peek(ProcessQ* q){
    return q->front;
}
/*
 * for periodic tasks
 * inserts at the appropriate point in the q
 */
void Q_Insert(ProcessQ* q, PD* pd) {
    if (q->length == 0) {
        pd->next = NULL;
        q->front = pd;
        q->back = pd;
        q->length++;
    }
    else if(pd->next_start < q->front->next_start){
        pd->next = q->front;
        q->front = pd;
        q->length++;
    }
    else {
        PD* prev = q->front;
		PD* cur = q->front->next;

        PD* spot = NULL;
	    	
		while(cur != NULL) {
            if(do_break){
                BIT_SET(SYSTEM_PORT_INIT,0);
                BIT_SET(SYSTEM_PORT, 0);
            }
			if(pd->next_start < cur->next_start) {
                spot =  prev;
                break;
			}
            prev = cur;
			cur = cur->next;
		}
        if(spot == NULL){
            q->back->next = pd;
            q->back = pd;
            q->back->next = NULL;
            q->length++;
        }
        else{
            pd->next = spot;
            spot->next = pd;
            q->length++;
        }
    }
}



