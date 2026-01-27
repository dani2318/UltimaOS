#pragma once
#include <stdint.h>
#include <arch/x86_64/Paging.hpp>

enum TaskState {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_TERMINATED
};

// CPU context saved during task switch
struct TaskContext
{
    uint64_t rsp;      // +0
    uint64_t rbp;      // +8
    uint64_t rbx;      // +16
    uint64_t r12;      // +24
    uint64_t r13;      // +32
    uint64_t r14;      // +40
    uint64_t r15;      // +48
    uint64_t rflags;   // +56
    
    // These are NOT used by switch_task but stored for reference:
    uint64_t cs;
    uint64_t ss;
    uint64_t ds;
    uint64_t rip;
    uint64_t rdi;
};

struct Task {
    uint64_t task_id;
    char name[32];
    TaskState state;
    
    TaskContext context;
    
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