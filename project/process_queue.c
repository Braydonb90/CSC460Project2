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
PD* Q_Pop(ProcessQ* q) {
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
    while(cur->next != NULL){
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
 * Standard q pop. Only used internally... or not used
 */
static PD* Q_Pop_Internal(ProcessQ* q) {
    if (q->length == 0) {
        return NULL;
    }

    PD* ret = q->front;
    q->front = q->front->next;
    q->length--;
    return ret;
}

/*
 * for periodic tasks
 */
void Q_Insert(ProcessQ* q, PD* pd) {
    //TODO
}



