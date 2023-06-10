#ifndef QUEUE_H
#define QUEUE_H

typedef struct node
{
    float value;       // Value stored in the node
    struct node *next; // Pointer to the next node in the linked list
} Node;

typedef struct queue
{
    Node *head;   // Pointer to the head of the queue
    Node *tail;   // Pointer to the tail of the queue
    int count;    // Number of elements currently in the queue
    int capacity; // Maximum capacity of the queue
} Queue;

Queue *initQueue(int capacity);
void enqueue(Queue *queue, float value);
float dequeue(Queue *queue);
int isFull(Queue *queue);
int isEmpty(Queue *queue);
void freeQueue(Queue *queue);

#endif /* QUEUE_H */