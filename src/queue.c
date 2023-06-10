#include <stdlib.h>
#include "queue.h"

// Queue initialization
Queue *initQueue(int capacity)
{
    Queue *queue = (Queue *)malloc(sizeof(Queue)); // Allocate memory for the Queue structure
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    queue->capacity = capacity;
    return queue;
}

// Adding elements to the queue
void enqueue(Queue *queue, float value)
{
    // Create a new node
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->value = value;
    newNode->next = NULL;
    
    // If the queue is full, return without adding the new node
    if (queue->count == queue->capacity)
    {
        return;
    }
    
    // If the queue is not empty, update the next pointer of the current tail to the new node
    if (queue->tail != NULL)
    {
        queue->tail->next = newNode;
    }
    
    // Update the tail of the queue to point to the new node
    queue->tail = newNode;
    
    // If the queue is empty, update the head to point to the new node as well
    if (queue->head == NULL)
    {
        queue->head = newNode;
    }
    
    queue->count++;
}

// Taking elements from the queue
float dequeue(Queue *queue)
{
    // Store the head node
    Node *temp = queue->head;
    
    // Store the value of the head node
    float value = temp->value;
    
    // If the queue is empty, return -1
    if (queue->count == 0)
    {
        return -1.0;
    }
    
    // Update the head of the queue to point to the next node
    queue->head = queue->head->next;
    
    if (queue->head == NULL)
    {
        queue->tail = NULL;
    }
    
    // Free the memory allocated for the dequeued node
    free(temp);
    
    queue->count--;
    
    return value;
}

int isFull(Queue *queue)
{
    // Check if the count of elements in the queue is equal to the queue's capacity
    return queue->count == queue->capacity;
}

int isEmpty(Queue *queue)
{
    // Check if the count of elements in the queue is 0
    return queue->count == 0;
}

void freeQueue(Queue *queue)
{
    Node *temp = queue->head;
    Node *nextNode;

    // Deallocate all nodes
    while (temp != NULL)
    {
        nextNode = temp->next;
        free(temp);
        temp = nextNode;
    }

    // Deallocate the queue
    free(queue);
}