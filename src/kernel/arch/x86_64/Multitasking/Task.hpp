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
struct TaskContext {
    // General purpose registers
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    
    // Segment registers
    uint64_t ds;
    
    // Stack and instruction pointers
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

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