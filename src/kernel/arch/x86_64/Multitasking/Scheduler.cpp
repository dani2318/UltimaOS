#include "Scheduler.hpp"
#include <arch/x86_64/Paging.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/Interrupts/GDT/GDT.hpp>
#include <globals.hpp>
#include <cpp/String.hpp>

Task *Scheduler::current_task = nullptr;
Task *Scheduler::task_list_head = nullptr;
uint64_t Scheduler::next_task_id = 0;

extern "C" void Scheduler_Schedule_Wrapper()
{
    Scheduler::Schedule();
}

extern "C" void switch_task(TaskContext *old_context, TaskContext *new_context);

void Scheduler::Initialize()
{
    g_screenwriter->Print("Scheduler: Initializing...\n\r");

    current_task = (Task *)g_PMM.RequestPage();
    memset(current_task, 0, sizeof(Task));

    current_task->task_id = next_task_id++;
    strcpy(current_task->name, "idle");
    current_task->state = TASK_RUNNING;
    current_task->priority = 0;
    current_task->time_slice = 10;
    current_task->time_remaining = 10;

    current_task->kernel_stack = (uint64_t)g_PMM.RequestPage();
    current_task->kernel_stack_size = 4096;

    asm volatile("mov %%cr3, %0" : "=r"(current_task->page_table));

    current_task->next = current_task;
    current_task->prev = current_task;
    task_list_head = current_task;
    task_list_head->next = task_list_head;
    task_list_head->prev = task_list_head;

    g_screenwriter->Print("Scheduler: Idle task created\n\r");
}

extern TSS g_tss;

Task *Scheduler::CreateTask(void (*entryPoint)(), const char *name, int priority, PageTable *pml4, void *arg)
{
    Task *task = (Task *)g_PMM.RequestPage();
    memset(task, 0, sizeof(Task));

    task->task_id = next_task_id++;
    strncpy(task->name, name, 31);
    task->name[31] = '\0';
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = priority * 10;
    task->time_remaining = task->time_slice;

    task->kernel_stack = (uint64_t)g_PMM.RequestPage();
    task->kernel_stack_size = 4096;

    // 1. Setup Stack Pointer (RSP)
    // We subtract 64 bytes to ensure iretq has room to push the frame
    // and to ensure the stack is 16-byte aligned for the C++ entry point.
    uint64_t stack_top = task->kernel_stack + task->kernel_stack_size;
    task->context.rsp = stack_top - 64;

    // 2. Setup Instruction Pointer and Arguments
    task->context.rdi = (uint64_t)arg; // First argument (KernelAPI*)
    task->context.rflags = 0x202;      // Interrupts enabled

    bool isUserTask = pml4 != kernelPLM4;

    // 3. Setup Selectors (Must match your GDT)
    if (isUserTask)
    {
        task->context.cs = 0x1B; // Index 3 (0x18) | RPL 3 = 0x1B
        task->context.ss = 0x23; // Index 4 (0x20) | RPL 3 = 0x23
        task->context.ds = 0x23; // Index 4 (0x20) | RPL 3 = 0x23
        task->context.rflags = 0x202;
        task->context.rsp = 0x70003FF0;
    }
    else
    {
        // This is a Kernel Task
        task->context.cs = 0x08;
        task->context.ss = 0x10;
        task->context.ds = 0x10;
        task->context.rsp = (uint64_t)g_PMM.RequestPage() + 4096 - 64;
    }

    // Page table
    task->context.rip = (uint64_t)entryPoint;
    task->context.rflags = 0x202; // IF bit set (interrupts on)
    task->page_table = pml4;

    // List linking
    if (task_list_head == nullptr)
    {
        task_list_head = task;
        task->next = task;
        task->prev = task;
    }
    else
    {
        task->next = task_list_head;
        task->prev = task_list_head->prev;
        task_list_head->prev->next = task;
        task_list_head->prev = task;
    }

    if (current_task != nullptr) {
        // Only switch if we have a valid source to save registers into
        g_tss.rsp0 = task->kernel_stack + task->kernel_stack_size;
        switch_task(&current_task->context, &task->context);
    } else {
        // If this is the first task (idle), just set it and continue
        current_task = task;
    }

    g_screenwriter->Print("Scheduler: Created task '");
    g_screenwriter->Print(name);
    g_screenwriter->Print("'\n\r");

    return task;
}

Task *Scheduler::SelectNextTask()
{
    Task *best_task = nullptr;
    // Start searching from the task AFTER the current one to ensure Round Robin
    Task *start_task = current_task->next;
    Task *task = start_task;

    do
    {
        if (task->state == TASK_READY || task->state == TASK_RUNNING)
        {
            // Check for higher priority, OR equal priority (Round Robin)
            if (best_task == nullptr || task->priority > best_task->priority)
            {
                best_task = task;
            }
        }
        task = task->next;
    } while (task != start_task);

    return (best_task == nullptr) ? task_list_head : best_task;
}

void Scheduler::Schedule()
{
    if (current_task == nullptr)
        return;

    // Decrease time remaining
    if (current_task->time_remaining > 0)
    {
        current_task->time_remaining--;
    }

    // If time slice expired or task yielded, switch
    if (current_task->time_remaining == 0 || current_task->state != TASK_RUNNING)
    {
        Task *next_task = SelectNextTask();

        if (next_task != current_task)
        {
            SwitchToTask(next_task);
        }
        else
        {
            // Reset time slice
            current_task->time_remaining = current_task->time_slice;
        }
    }
}

void Scheduler::SwitchToTask(Task *next_task)
{
    if (next_task == nullptr || next_task == current_task)
        return;

    Task *old_task = current_task;
    if (old_task->state == TASK_RUNNING)
        old_task->state = TASK_READY;

    next_task->state = TASK_RUNNING;
    next_task->time_remaining = next_task->time_slice;
    current_task = next_task;

    // CR3 Switch
    if (old_task->page_table != next_task->page_table)
    {
        asm volatile("mov %0, %%cr3" : : "r"(next_task->page_table));
    }

    switch_task(&old_task->context, &next_task->context);
}

void Scheduler::Yield()
{
    if (current_task)
    {
        current_task->time_remaining = 0;
        Schedule();
    }
}

Task *Scheduler::GetCurrentTask()
{
    return current_task;
}

void Scheduler::ExitTask()
{
    if (current_task)
    {
        current_task->state = TASK_TERMINATED;
        Yield();
    }
}
