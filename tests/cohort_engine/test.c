// test_cohort.c
#include <stdint.h>

#define QUEUE_ADDR 0x90000000

int main() {
    volatile uint64_t *queue = (volatile uint64_t *)QUEUE_ADDR;

    // Write a task to shared memory
    queue[0] = 42;

    // Loop forever to keep simulation alive
    while (1);
}
