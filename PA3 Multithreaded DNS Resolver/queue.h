#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

#define QUEUE_FAILURE -1
#define QUEUE_SUCCESS 0

typedef struct queueNode_s
{
	void* payload;
} queueNode;

typedef struct queue_s
{
    	queueNode* array;
    	int front;
    	int back;
    	int maxSize;
} queue;

/* Function to initilze a new queue
 * On success, returns queue size
 * On failure, returns QUEUE_FAILURE
 * Must be called before queue is used
 */
int queue_init(queue* q, int size);

/* Function to test if queue is empty
 * Returns 1 if empty, 0 otherwise
 */
int queue_is_empty(queue* q);

/* Function to test if queue is full
 * Returns 1 if full, 0 otherwise
 */
int queue_is_full(queue* q);

/* Function add payload to end of FIFO queue
 * Returns QUEUE_SUCCESS if the push successeds.
 * Returns QUEUE_FAILURE if the push fails
 */
int queue_push(queue* q, void* payload);

/* Function to return element from queue in FIFO order
 * Returns NULL pointer if queue is empty
 */
void* queue_pop(queue* q);

/* Function to free queue memory */
void queue_cleanup(queue* q);

#endif