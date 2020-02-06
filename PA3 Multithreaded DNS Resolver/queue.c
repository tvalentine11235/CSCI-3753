/* This queue was developed by Chris Waile, Wei-Te Chen, and Andy Sayler
and was taken and used, with slight modifications by myself, from their
public github account at 
https://github.com/AHAAAAAAA/CSCI3753-OperatingSystems/blob/master/Lab3/pa3-files/queue.c */

#include <stdlib.h>

#include "queue.h"

int queue_init (queue* q, int size)
{
    
	int i;
	
	/* Determine the size of the Queue */
	if(size>0) 
	{
		q->maxSize = size;
    	}
    	else 
	{
		q->maxSize = 1025;
	}

	/* Allocate memory for the array */
	q->array = malloc(sizeof(queueNode) * (q->maxSize));
	if(!(q->array))
	{	
		perror("Error on queue Malloc");
		return QUEUE_FAILURE;
	}

	/* Set all of our queue values to NULL */
	for(i=0; i < q->maxSize; ++i)
	{
		q->array[i].payload = NULL;
	}

	/* Front and back of queue */
	q->front = 0;
	q->back = 0;

    return q->maxSize;
}

int queue_is_empty (queue* q)
/* Simple check to determine if front and back are the same and there is no payload inside */
{
	if((q->front == q->back) && (q->array[q->front].payload == NULL))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int queue_is_full (queue* q)
/* Simple check to see if front and back are the same and there is a payload inside */
{
	if((q->front == q->back) && (q->array[q->front].payload != NULL))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void* queue_pop (queue* q)
/* Return the front element from the queue */
{
 	void* returnPayload;
	
	if(queue_is_empty(q))
	{
		return NULL;
    	}
	
    	returnPayload = q->array[q->front].payload;
    	q->array[q->front].payload = NULL;
    	q->front = ((q->front + 1) % q->maxSize);

 	return returnPayload;
}

int queue_push (queue* q, void* new_payload)
{
    	if(queue_is_full(q))
	{
		return QUEUE_FAILURE;
    	}
	
	q->array[q->back].payload = new_payload;
	q->back = ((q->back+1) % q->maxSize);
	return QUEUE_SUCCESS;
}

void queue_cleanup(queue* q)
{
	while(!queue_is_empty(q))
	{
		queue_pop(q);
    	}
    	free(q->array);
}

