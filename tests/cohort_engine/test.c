// test_cohort.c
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

#define QUEUE_ADDR 0x10000000
#define SIZE 0x0FFFFFFF 
#define COMPUTE_QUEUE_ADDR 0x10000000
#define RESULT_QUEUE_ADDR 0x11000000
#define COHORT_REGISTRATION_ADDR 0x12000000
#define QUEUE_CAPACITY 32


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

uint64_t size(fifo_t *q) {
    return (*(q->tail))-(*(q->head));
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

typedef struct {
    uint64_t value;
    uint64_t tick;
} entry_t;

uint64_t get_tick() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void fake_sleep(uint64_t delay_ns) {
    uint64_t start = get_tick();
    while (get_tick() - start < delay_ns) {
        // spin
    }
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

     
    fifo_t *in_queue = fifo_init(sizeof(entry_t), QUEUE_CAPACITY, COMPUTE_QUEUE_ADDR);
    fifo_t *out_queue = fifo_init(sizeof(entry_t), QUEUE_CAPACITY, RESULT_QUEUE_ADDR);

    cohort_register(12, in_queue, out_queue);

    uint64_t input_values[] = {45, 48, 10086, 10492, 114514};
    int num_values = sizeof(input_values) / sizeof(input_values[0]);

    for (int i = 0; i < num_values; ++i) {
        entry_t e;
        e.value = input_values[i];
        e.tick = get_tick();
        push(e.value, in_queue);
        printf("Pushed 0x%lx at tick %lu\n", e.value, e.tick);
        fake_sleep(50000);  // simulate ~50ms delay between pushes
    }

    int received = 0;
    while (received < num_values) {
        if (size(out_queue) > 0) {
            entry_t result;
            result.value = pop(out_queue);
            result.tick = get_tick();
            printf("Received 0x%lx | Latency: %lu ns\n", result.value, result.tick);
            ++received;
        } else {
            fake_sleep(1000);  // 1ms polling
        }
    }

    cohort_unregister(12, in_queue, out_queue);
    
    return 0;
}
