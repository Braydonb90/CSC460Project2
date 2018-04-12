#include "process_queue.h"
#include "output.h"


static void print_queue(ProcessQ* q);

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
    if(DEBUG) puts("|\n|\n");
    if(DEBUG) printf("Q Push\n");
    print_queue(q); 
	pd->next = NULL;
    if (q->length == 0) {
        q->front = pd;
    }
    else {
        q->back->next = pd;
    }
    q->back = pd;
    q->length++;
    if(DEBUG) printf("Push %d\n", pd->pid);
    if(DEBUG) puts("|\n|\n");
}

PD* Q_Pop_Ready(ProcessQ* q) {
    if(DEBUG) puts("|\n|\n");
	if(DEBUG) printf("Q Pop Ready\n");
	print_queue(q);
	
	PD* prev = NULL;
    PD* cur = q->front;
	
	while(cur != NULL) {
		if(cur->state == READY) {
			q->length--;
			
			if(q->front->pid == cur->pid) {
				q->front = cur->next;
			}
			if(q->back->pid == cur->pid) {
				q->back = prev;
			}
			if(prev != NULL) {
				prev->next = cur->next;
			}
			break;
		}
		prev = cur;
		cur = cur->next;
	}
	if(cur != NULL) {
		if(DEBUG) printf("Pop %d\n", cur->pid);
	} else {
		if(DEBUG) printf("Pop nothing\n");
	}
    if(DEBUG) puts("|\n|\n");
	return cur;
}

/*
 * Cycles through Q, returning the first READY task
 */
PD* Q_Pop_Ready_OLD(ProcessQ* q) {
    int len = q->length,i;
    PD* prev = q->front;
    PD* cur = prev->next;
    //in case the front is actually ready
    if(prev->state == READY){
        //cur should be null here???
        if(q->front == q->back){
            if(cur != NULL){
                OS_Abort(NON_NULL_Q_BACK);
            }
            q->back = cur;
        }
        q->front = cur;
        q->length--;
        return prev;
    }
    //otherwise
    while(cur != NULL){
        if(cur->state == READY){
            if(cur == q->back){
                q->back = prev;
            }
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

void print_queue(ProcessQ* q) {
	int i = 0;
	PD* cur = q->front;
	while(cur != NULL){
		if(DEBUG) printf("----- Index %d -----\n", i);  
		if(DEBUG) printf("PID: %d | state: %d | priority: %d", cur->pid, cur->state, cur->priority);
		i++;
        cur = cur->next;
		if(cur != NULL){
			if(DEBUG) printf(" | next: %d\n", cur->pid);
		}
        else {
            if(DEBUG) puts(" | next: NULL\n");
        }
    }  
	if(DEBUG) printf("Queue size %d\n", i);  
	if(DEBUG) printf("Queue length %d\n", q->length);  
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
    if(DEBUG) puts("|\n|\n");
    if(DEBUG) printf("Q Insert\n");
    print_queue(q);
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
    if(DEBUG) puts("|\n|\n");
}
unsigned int Q_CountScheduledTasks(ProcessQ* q, unsigned int elapsed){
    int count = 0;
    PD* p = q->front;
    while(p != NULL){
        if(p->next_start < elapsed){
            count++;
        }
        p = p->next;
    }
    return count;
}



