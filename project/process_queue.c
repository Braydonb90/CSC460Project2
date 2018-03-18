#include "process_queue.h"

ProcessQ* Q_Init(ProcessQ* q, PRIORITY type){
    q->front = NULL;
    q->back = NULL;
    q->length = 0;
    q->queue_type = type;

    return q;
}

/*
 * for RR tasks
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

PD* Q_Pop(ProcessQ* q) {
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



