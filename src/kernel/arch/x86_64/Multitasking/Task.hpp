#pragma once
#include <stdint.h>
#include <arch/x86_64/Paging/Paging.hpp>

enum TaskState {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_TERMINATED
};

struct Task_Registers{
    uint32_t eax, ebx, ecx, edx, edi, esp, ebp, eip, eflags, cr3;
};

typedef struct Task_Registers Task_Registers;

struct Task {
    uint64_t task_id;
    char name[32];
    TaskState state;
    
    Task_Registers context;
    
    // Stack
    uint64_t kernel_stack;
    uint64_t kernel_stack_size;
    
    // Page table
    PageTable* page_table;
    
    // Linked list (for all tasks)
    Task* next;
    Task* prev;
    
    // ========== CFS Scheduling Fields ==========
    
    // Virtual runtime: tracks how much CPU time this task has consumed
    // Task with lowest vruntime gets to run next
    uint64_t vruntime;
    
    // Weight/Nice value: determines CPU share
    // Higher weight = more CPU time = slower vruntime growth
    // Default (nice 0) = 1024
    // Range: 15 (nice +19) to 88761 (nice -20)
    uint32_t weight;
    
    // For tracking actual execution time
    uint64_t sum_exec_runtime;  // Total time this task has run
    uint64_t exec_start;        // When current execution period started (in timer ticks)
    
    // Minimum granularity: minimum time a task runs before preemption (in timer ticks)
    // Prevents excessive context switching
    uint64_t min_granularity;
    
    // For I/O handling - when a task blocks, we store its sleep start time
    uint64_t sleep_start;
    
    // Red-Black Tree node fields (for CFS run queue)
    // In a simple implementation, we can use linked list with sorted insertion
    // For now, keeping the circular list but will sort by vruntime
    bool on_rq;  // Is this task on the run queue?
};

// Nice value to weight conversion table (matches Linux CFS)
// Nice values: -20 (highest priority) to +19 (lowest priority)
// Default nice = 0, weight = 1024
static const uint32_t prio_to_weight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548, 7620, 6100, 4904, 3906,
    /*  -5 */ 3121, 2501, 1991, 1586, 1277,
    /*   0 */ 1024, 820, 655, 526, 423,
    /*   5 */ 335, 272, 215, 172, 137,
    /*  10 */ 110, 87, 70, 56, 45,
    /*  15 */ 36, 29, 23, 18, 15
};

// Convert nice value (-20 to +19) to weight
inline uint32_t nice_to_weight(int nice) {
    if (nice < -20) nice = -20;
    if (nice > 19) nice = 19;
    return prio_to_weight[nice + 20];
}
