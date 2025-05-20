// test_cohort.c
#include <stdint.h>
#include <stddef.h>

#define QUEUE_ADDR 0x90000000

typedef struct {
    uint64_t head;
    uint64_t tail;
    uint64_t capacity;
    uint64_t element_size;
    uint8_t data[];  // Flexible array member
} fifo_t;

fifo_t *fifo_init(int element_size, int queue_length) {
    size_t total_size = sizeof(fifo_t) + element_size * queue_length;
    fifo_t *q = (fifo_t *) 0x90001000;  // static address in DRAM

    // Clear everything
    for (int i = 0; i < total_size / sizeof(uint64_t); i++)
        ((uint64_t *)q)[i] = 0;

    q->capacity = queue_length;
    q->element_size = element_size;
    return q;
}

void push(uint64_t element, fifo_t *q) {
    uint64_t tail = q->tail;
    uint64_t idx = tail % q->capacity;
    uint64_t *data = (uint64_t *)(q->data);
    data[idx] = element;
    q->tail++;
}

int fifo_deinit(fifo_t *q) {
    // no-op in SE mode
    return 0;
}

typedef struct {
    uint64_t acc_id;
    uint64_t in_addr;
    uint64_t out_addr;
} cohort_registration_t;

#define COHORT_REGISTRATION_ADDR 0x9000F000

int cohort_register(int acc_id, fifo_t *acc_in, fifo_t *acc_out) {
    cohort_registration_t *entry = (cohort_registration_t *) COHORT_REGISTRATION_ADDR;
    entry->acc_id = acc_id;
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

     

    volatile uint64_t *queue = (uint64_t *)QUEUE_ADDR;

    printf("Writing to Cohort queue...\n");
    queue[0] = 0x12345678abcdef00;

    printf("Reading from Cohort queue: 0x%lx\n", queue[0]);
     /*
     // Initialize input and output queues
     fifo_t *in_queue = fifo_init(sizeof(uint64_t), 8);
     fifo_t *out_queue = fifo_init(sizeof(uint64_t), 8);  // Optional if your engine returns results
 
     // Register with cohort engine
     cohort_register(1, in_queue, out_queue);
 
     // Push some tasks
     for (uint64_t i = 0; i < 5; ++i) {
         push(i * 10, in_queue);
     }
 
     // Keep simulation running
     while (1);
 
     // (Unregister is unreachable but shows cleanup API)
     cohort_unregister(1, in_queue, out_queue);
     fifo_deinit(in_queue);
     fifo_deinit(out_queue);
    */
     return 0;
}
