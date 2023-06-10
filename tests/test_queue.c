#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include "../src/queue.h"

void test_queue()
{
    // Initialize the queue
    Queue *queue = initQueue(5);
    assert(queue != NULL);

    // Enqueue values
    enqueue(queue, 10.5);   // Enqueue the value 10.5
    enqueue(queue, 36.6);   // Enqueue the value 36.6
    enqueue(queue, 105.6);  // Enqueue the value 105.6
    enqueue(queue, 123.1);   // Enqueue the value 123.1
    enqueue(queue, 177.7);  // Enqueue the value 177.7

    // Check if the queue is empty
    assert(isFull(queue));

    // Dequeue values and check if they match the expected values
    float value = dequeue(queue);
    // Due to decision issues, I decided to check the condition if the values are approximately equal to each other
    assert(fabs(value - 10.5) < 0.0001);   // Check if the dequeued value is approximately equal to 10.5
    value = dequeue(queue);
    assert(fabs(value - 36.6) < 0.0001);   // Check if the dequeued value is approximately equal to 36.6
    value = dequeue(queue);
    assert(fabs(value - 105.6) < 0.0001);  // Check if the dequeued value is approximately equal to 105.6
    value = dequeue(queue);
    assert(fabs(value - 123.1) < 0.0001);   // Check if the dequeued value is approximately equal to 123.1
    value = dequeue(queue);
    assert(fabs(value - 177.7) < 0.0001);  // Check if the dequeued value is approximately equal to 177.7

    // Check if the queue is empty
    assert(isEmpty(queue));

    // Free the queue
    freeQueue(queue);
}

int main()
{
    // Run the unit tests
    test_queue();

    printf("All tests passed!\n");

    return 0;
}
