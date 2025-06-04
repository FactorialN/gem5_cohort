// test_cohort.c
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

#define QUEUE_ADDR 0x10000000
#define SIZE 0x0FFFFFFF 
#define COMPUTE_QUEUE_ADDR 0x10000000
#define RESULT_QUEUE_ADDR 0x11000000
#define COHORT_REGISTRATION_ADDR 0x12000000


typedef struct {
    volatile uint64_t * head;
    volatile uint64_t * tail;
    uint64_t capacity;
    uint64_t element_size;
    uint8_t data[];  // Flexible array member
} fifo_t;

void print_fifo(fifo_t *q) {
    printf("=== FIFO State ===\n");
    printf("head address: 0x%lx\n", (uint64_t)q->head);
    printf("tail address: 0x%lx\n", (uint64_t)q->tail);
    printf("head value  : 0x%lx\n", *(q->head));
    printf("tail value  : 0x%lx\n", *(q->tail));
    printf("capacity    : %d\n", q->capacity);
    printf("element_size: %d\n", q->element_size);

    printf("Queue content (raw uint64_t):\n");
    for (int i = 0; i < q->capacity; i++) {
        uint64_t *slot = (uint64_t *)((char *)q->head + 16 + i * q->element_size);
        printf("  [%d] = 0x%lx\n", i, *slot);
    }
}

fifo_t *fifo_init(int element_size, int queue_length, uint64_t addr) {
    size_t total_size = sizeof(fifo_t) + element_size * queue_length;
    fifo_t *q = malloc(sizeof(fifo_t)); 

    fifo_t *mem = (fifo_t *) addr;  // static address in DRAM

    // Clear everything
    for (int i = 0; i < total_size / sizeof(uint64_t); i++)
        ((uint64_t *)mem)[i] = 0;

    // address of memory location that saves head and tail
    q->head = (volatile uint64_t *)addr;
    q->tail = (volatile uint64_t *)(addr + 8);
    // value of memory location that saves head and tail
    // addr + 2 at first
    *(q->head) = addr + 16;
    // empty queue at first
    *(q->tail) = addr + 16;
    q->capacity = queue_length;
    q->element_size = element_size;

    return q;
}

void push(uint64_t element, fifo_t *q) {
    *((volatile uint64_t *)(*(q->tail))) = element;
    *(q->tail) += 8;
}

uint64_t pop(fifo_t *q) {
    uint64_t val = *((volatile uint64_t *)(*(q->head)));
    *(q->head) += 8;
    return val;
}


typedef struct {
    uint64_t acc_id;
    uint64_t in_addr;
    uint64_t out_addr;
} cohort_registration_t;

int cohort_register(int acc_id, fifo_t *acc_in, fifo_t *acc_out) {
    cohort_registration_t *entry = (cohort_registration_t *) COHORT_REGISTRATION_ADDR;
    entry->acc_id = acc_id;
    printf("ACC ID ADDR %p\n", (void *)&entry->acc_id);
    entry->in_addr = (uint64_t) acc_in;
    entry->out_addr = (uint64_t) acc_out;
    return 0;
}

int cohort_unregister(int acc_id, fifo_t *acc_in, fifo_t *acc_out) {
    cohort_registration_t *entry = (cohort_registration_t *) COHORT_REGISTRATION_ADDR;
    entry->acc_id = 0;
    return 0;
}

int main() {

    void *mapped = mmap(QUEUE_ADDR, SIZE,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                        -1, 0);

    if (mapped == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

     

    volatile uint64_t *queue = (uint64_t *)QUEUE_ADDR;

    printf("Writing to Cohort queue...\n");
    queue[0] = 42;

    printf("Reading from Cohort queue: 0x%lx\n", queue[0]);
     
     // Initialize input and output queues
    fifo_t *in_queue = fifo_init(sizeof(uint64_t), 32, COMPUTE_QUEUE_ADDR);
    fifo_t *out_queue = fifo_init(sizeof(uint64_t), 32, RESULT_QUEUE_ADDR);  // Optional if your engine returns results
    //print_fifo(in_queue);

    cohort_register(12, in_queue, out_queue);

    push(45, in_queue);
    push(48, in_queue);
    push(10086, in_queue);
    push(10492, in_queue);

    //print_fifo(in_queue);
    //print_fifo(out_queue);

    while((*(out_queue->head))<(*(out_queue->tail))){
        printf("Poping from Cohort out queue: 0x%lx\n", pop(out_queue));
    }

    cohort_unregister(1, in_queue, out_queue);
    
     return 0;
}
