#include "process_queue.h"
#include "output.h"

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
    if(DEBUG) print_queue(q); 
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

/*
 * Cycles through Q, returning the first READY task
 */
PD* Q_Pop_Ready(ProcessQ* q) {
    if(DEBUG) puts("|\n|\n");
	if(DEBUG) printf("Q Pop Ready\n");
	if(DEBUG) print_queue(q);
	
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

void print_queue(ProcessQ* q) {
	int i = 0;
	PD* cur = q->front;
	while(cur != NULL){
		printf("----- Index %d -----\n", i);  
		printf("PID: %d | state: %d | priority: %d", cur->pid, cur->state, cur->priority);
        if(q->queue_type == PERIODIC) {
            printf(" | next_start: %d", cur->next_start);
        }
		i++;
        cur = cur->next;
		if(cur != NULL){
			printf(" | next: %d\n", cur->pid);
		}
        else {
            puts(" | next: NULL\n");
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
    if(DEBUG) print_queue(q);
	pd->next = NULL;
    if (q->length == 0) {
        q->front = pd;
        q->back = pd;
    }
    else if(pd->next_start < q->front->next_start){
        pd->next = q->front;
        q->front = pd;
    }
    else {
        PD* prev = q->front;
		PD* cur = q->front->next;

        PD* spot = NULL;
	    	
		while(cur != NULL) {
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
        }
        else{
			pd->next = spot->next;
            spot->next = pd;
        }
    }
    q->length++;
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



