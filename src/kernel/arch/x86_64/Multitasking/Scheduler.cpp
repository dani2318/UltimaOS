#include "Scheduler.hpp"
#include <cpp/cppSupport.hpp>

extern "C" void switch_context(Task_Registers* old_context, Task_Registers* new_context);

extern "C" void* kmalloc(uint64_t size);
extern "C" void kfree(void* ptr);

Scheduler::Scheduler()
    : readyQueueHead(nullptr), currentTask(nullptr), nextTaskID(1) {
}

void Scheduler::addToReadyQueue(Task* task){
    if(!task) return;

    task->state = TASK_READY;

    if(!readyQueueHead){
        readyQueueHead = task;
        task->next = task;
        task->prev = task;
        return;
    }

    Task* tail = readyQueueHead->prev;
    tail->next = task;
    task->prev = tail;  // Fixed: was tail->prev = tail
    task->next = readyQueueHead;
    readyQueueHead->prev = task;
}

void Scheduler::removeFromReadyQueue(Task* task){
    if(!task || !readyQueueHead) return;

    if(task->next == task){
        readyQueueHead = nullptr;
        return;
    }

    task->prev->next = task->next;
    task->next->prev = task->prev;

    if(readyQueueHead == task){
        readyQueueHead = task->next;
    }

    task->next = nullptr;
    task->prev = nullptr;
}

Task* Scheduler::getNextTask(){
    if(!readyQueueHead) return nullptr;

    Task* bestTask = readyQueueHead;
    Task* current = readyQueueHead->next;

    while (current != readyQueueHead)
    {
        if(current->priority > bestTask->priority){
            bestTask = current;
        }
        current = current->next;
    }
    
    readyQueueHead = readyQueueHead->next;
    return bestTask;
}

Task* Scheduler::CreateTask(const char* name, void (*entry_point)(), uint64_t priority){
    Task* task = (Task*)kmalloc(sizeof(Task));
    if(!task) return nullptr;

    task->task_id = nextTaskID++;
    strcpy(task->name, name);
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = 10;
    task->time_remaining = task->time_slice;

    // Allocate kernel stack (align to 16 bytes for x86-64)
    task->kernel_stack_size = 8192;  // Larger stack for 64-bit
    task->kernel_stack = (uint64_t)kmalloc(task->kernel_stack_size);
    
    if(!task->kernel_stack) {
        kfree(task);
        return nullptr;
    }

    // Initialize context (32-bit registers but 64-bit addresses)
    memset(&task->context, 0, sizeof(Task_Registers));
    
    task->context.eax = 0;
    task->context.ebx = 0;
    task->context.ecx = 0;
    task->context.edx = 0;
    task->context.edi = 0;
    task->context.ebp = 0;
    task->context.eflags = 0x202;  // IF (interrupts enabled)
    
    // Set entry point (truncate to 32-bit)
    task->context.eip = (uint32_t)(uint64_t)entry_point;
    
    // Set stack pointer (top of stack, grows downward, 16-byte aligned)
    uint64_t stack_top = task->kernel_stack + task->kernel_stack_size;
    stack_top &= ~0xFULL;  // Align to 16 bytes
    task->context.esp = (uint32_t)stack_top;

    // Page table will be set by caller if needed
    task->page_table = nullptr;

    task->next = nullptr;
    task->prev = nullptr;

    addToReadyQueue(task);

    return task;
}

void Scheduler::DestroyTask(uint64_t taskID){
    Task* task = findTask(taskID);
    if(!task) return;

    if(task == currentTask)
        currentTask = nullptr;

    removeFromReadyQueue(task);

    if(task->kernel_stack)
        kfree((void*)task->kernel_stack);
    
    kfree(task);

    if(!currentTask)
        Schedule();
}

void Scheduler::Schedule() {
    Task* prev_task = currentTask;
    Task* next_task = getNextTask();
    
    if (!next_task) return;
    
    // If previous task is still runnable, add back to ready queue
    if (prev_task && prev_task->state == TASK_RUNNING) {
        prev_task->time_remaining = prev_task->time_slice;
        addToReadyQueue(prev_task);
    }
    
    // Remove next task from ready queue and mark as running
    removeFromReadyQueue(next_task);
    next_task->state = TASK_RUNNING;
    currentTask = next_task;
    
    // Context switch
    if (prev_task && prev_task != next_task) {
        switch_context(&prev_task->context, &next_task->context);
    } else if (!prev_task) {
        // First task - manually set up context
        // Load CR3 if needed
        if (next_task->page_table) {
            asm volatile("mov %0, %%cr3" :: "r"((uint64_t)next_task->page_table));
        }
        
        // Jump to task entry point
        void (*entry)() = (void(*)())(uint64_t)next_task->context.eip;
        entry();
    }
}

void Scheduler::Yield() {
    if (!currentTask) return;
    
    currentTask->state = TASK_READY;
    Schedule();
}

void Scheduler::BlockCurrentTask() {
    if (!currentTask) return;
    
    currentTask->state = TASK_BLOCKED;
    Schedule();
}

void Scheduler::UnblockTask(uint64_t task_id) {
    Task* task = findTask(task_id);
    if (!task || task->state != TASK_BLOCKED) return;
    
    addToReadyQueue(task);
}

Task* Scheduler::findTask(uint64_t task_id) {
    if (currentTask && currentTask->task_id == task_id) {
        return currentTask;
    }
    
    if (!readyQueueHead) return nullptr;
    
    Task* t = readyQueueHead;
    do {
        if (t->task_id == task_id) return t;
        t = t->next;
    } while (t != readyQueueHead);
    
    return nullptr;
}

void Scheduler::timerTick() {
    if (!currentTask) {
        Schedule();
        return;
    }
    
    currentTask->time_remaining--;
    
    if (currentTask->time_remaining == 0) {
        Yield();
    }
}

// Wrapper for timer interrupt
extern "C" void Scheduler_Schedule_Wrapper() {
    // Assuming global scheduler instance
    extern Scheduler* g_scheduler;
    if (g_scheduler) {
        g_scheduler->timerTick();
    }
}