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
    
    // Linked list
    Task* next;
    Task* prev;
    
    // Priority and time slice
    uint64_t priority;
    uint64_t time_slice;
    uint64_t time_remaining;
};