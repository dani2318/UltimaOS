#include "Scheduler.hpp"
#include <arch/x86_64/Paging.hpp>
#include <arch/x86_64/Serial.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/Interrupts/GDT/GDT.hpp>
#include <globals.hpp>
#include <cpp/String.hpp>

Task *Scheduler::current_task = nullptr;
Task *Scheduler::task_list_head = nullptr;
uint64_t Scheduler::next_task_id = 0;

extern "C" void Scheduler_Schedule_Wrapper()
{
    asm volatile("cli");
    g_serialWriter->Print("Timer interrupt!\n");
    Scheduler::Schedule();
    g_serialWriter->Print("Timer done\n");
}

extern "C" void switch_task(TaskContext *old_context, TaskContext *new_context, PageTable *pml4);

void Scheduler::Initialize()
{
    current_task = (Task *)g_PMM.RequestPage();
    memset(current_task, 0, sizeof(Task));

    // Save the current stack pointer
    uint64_t current_rsp;
    asm volatile("mov %%rsp, %0" : "=r"(current_rsp));
    current_task->context.rsp = current_rsp;

    // Use the explicit global kernel page table instead of reading CR3
    current_task->page_table = kernelPLM4; 

    strcpy(current_task->name, "idle");
    current_task->state = TASK_RUNNING;
    current_task->priority = 0;
    current_task->time_slice = 10;
    current_task->time_remaining = 10;

    current_task->kernel_stack = (uint64_t)g_PMM.RequestPage();
    current_task->kernel_stack_size = 4096;

    current_task->next = current_task;
    current_task->prev = current_task;
    task_list_head = current_task;
    task_list_head->next = task_list_head;
    task_list_head->prev = task_list_head;

    g_screenwriter->Print("Scheduler: Idle task created\n\r");
}

extern "C" void user_task_trampoline();

Task *Scheduler::CreateTask(void (*entryPoint)(), const char *name, int priority,
                            PageTable *pml4, void *userStackTop)
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

    // Allocate kernel stack for this task
    task->kernel_stack = (uint64_t)g_PMM.RequestPage();
    task->kernel_stack_size = 4096;
    
    // ============ FIX #4: Map critical structures into task's page table ============
    // CRITICAL: For user tasks, map kernel stack and task structure into their page table
    bool isUserTask = (pml4 != nullptr && pml4 != kernelPLM4);
    
    if (isUserTask) {
        PageTableManager tempManager;
        tempManager.Init(pml4);
        
        g_serialWriter->Print("Mapping task structures for '");
        g_serialWriter->Print(name);
        g_serialWriter->Print("'\n");
        
        // 1. Map the Task structure itself (scheduler needs to access this)
        tempManager.MapMemory((void*)task, (void*)task, false);
        g_serialWriter->Print("  Task struct at 0x");
        g_serialWriter->PrintHex((uint64_t)task);
        g_serialWriter->Print("\n");
        
        // 2. Map the kernel stack (identity mapped, kernel-only access)
        tempManager.MapMemory((void*)task->kernel_stack, (void*)task->kernel_stack, false);
        g_serialWriter->Print("  Kernel stack at 0x");
        g_serialWriter->PrintHex(task->kernel_stack);
        g_serialWriter->Print("\n");
        
        // 3. Map the current task list (scheduler accesses this)
        if (task_list_head != nullptr) {
            tempManager.MapMemory((void*)task_list_head, (void*)task_list_head, false);
        }
        
        // 4. Map current_task pointer location (global variable)
        uint64_t current_task_addr = (uint64_t)&current_task;
        tempManager.MapMemory((void*)(current_task_addr & ~0xFFFULL), 
                             (void*)(current_task_addr & ~0xFFFULL), false);
    }
    // ============ END OF FIX #4 ============
    
    uint64_t kstack_top = task->kernel_stack + task->kernel_stack_size;

    if (isUserTask)
    {
        uint64_t *stack = (uint64_t *)kstack_top;

        // 1. Setup the IRET frame (Used by user_task_trampoline)
        *(--stack) = 0x23;                   // SS (User Data)
        *(--stack) = (uint64_t)userStackTop; // RSP
        *(--stack) = 0x202;                  // RFLAGS (IF bit set)
        *(--stack) = 0x1B;                   // CS (User Code)
        *(--stack) = (uint64_t)entryPoint;   // RIP

        // 2. Setup the switch_task return address
        // This MUST be the last thing pushed so 'ret' in switch_task jumps here
        *(--stack) = (uint64_t)user_task_trampoline;

        task->context.rsp = (uint64_t)stack;
    }
    else
    {
        // Kernel task: simply push entryPoint for the 'ret' in switch_task
        uint64_t *stack = (uint64_t *)kstack_top;
        *(--stack) = (uint64_t)entryPoint;
        task->context.rsp = (uint64_t)stack;
    }

    task->page_table = pml4;

    // Add to circular task list
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

    // Set current task if none exists
    if (current_task == nullptr)
    {
        current_task = task;
        task->state = TASK_RUNNING;
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
    {
        g_serialWriter->Print("Schedule: No current task\n");
        return;
    }

    g_serialWriter->Print("Schedule: Current=");
    g_serialWriter->Print(current_task->name);
    g_serialWriter->Print("\n");

    // Decrease time remaining
    if (current_task->time_remaining > 0)
    {
        current_task->time_remaining--;
    }

    // If time slice expired or task yielded, switch
    if (current_task->time_remaining == 0 || current_task->state != TASK_RUNNING)
    {
        g_serialWriter->Print("Schedule: Time expired, selecting next\n");
        Task *next_task = SelectNextTask();

        g_serialWriter->Print("Schedule: Selected ");
        g_serialWriter->Print(next_task->name);
        g_serialWriter->Print("\n");

        if (next_task != current_task)
        {
            g_serialWriter->Print("Schedule: Switching...\n");
            SwitchToTask(next_task);
            g_serialWriter->Print("Schedule: Switch complete\n");
        }
        else
        {
            g_serialWriter->Print("Schedule: Same task, resetting time\n");
            current_task->time_remaining = current_task->time_slice;
        }
    }
}

void Scheduler::SwitchToTask(Task *next_task)
{
    if (next_task == nullptr || next_task == current_task)
        return;

    Task *old_task = current_task;

    // VALIDATION
    g_serialWriter->Print("Pre-switch validation:\n");
    g_serialWriter->Print("  Old task: ");
    g_serialWriter->Print(old_task->name);
    g_serialWriter->Print(" RSP=0x");
    g_serialWriter->PrintHex(old_task->context.rsp);
    g_serialWriter->Print("\n");

    g_serialWriter->Print("  New task: ");
    g_serialWriter->Print(next_task->name);
    g_serialWriter->Print(" RSP=0x");
    g_serialWriter->PrintHex(next_task->context.rsp);
    g_serialWriter->Print("\n");

    if (old_task->state == TASK_RUNNING)
        old_task->state = TASK_READY;

    next_task->state = TASK_RUNNING;
    next_task->time_remaining = next_task->time_slice;
    current_task = next_task;

    g_tss.rsp0 = (next_task->kernel_stack + next_task->kernel_stack_size) & ~0xF;
    g_serialWriter->Print("Calling switch_task with CR3 update...\n");
    switch_task(&old_task->context, &next_task->context, next_task->page_table);
    g_serialWriter->Print("Returned from switch_task\n");
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
