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
uint64_t Scheduler::system_ticks = 0;

extern "C" void Scheduler_Schedule_Wrapper()
{
    asm volatile("cli");
    Scheduler::IncrementSystemTicks();  // Increment global tick counter
    Scheduler::Schedule();
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

    // CFS initialization
    current_task->weight = nice_to_weight(0);  // Normal priority
    current_task->vruntime = 0;
    current_task->sum_exec_runtime = 0;
    current_task->exec_start = 0;
    current_task->min_granularity = MIN_GRANULARITY;
    current_task->on_rq = true;

    current_task->kernel_stack = (uint64_t)g_PMM.RequestPage();
    current_task->kernel_stack_size = 4096;

    current_task->next = current_task;
    current_task->prev = current_task;
    task_list_head = current_task;

    g_screenwriter->Print("Scheduler: CFS initialized with idle task\n\r");
}

extern "C" void user_task_trampoline();

Task *Scheduler::CreateTask(void (*entryPoint)(), const char *name, int nice,
                            PageTable *pml4, void *userStackTop)
{
    Task *task = (Task *)g_PMM.RequestPage();
    memset(task, 0, sizeof(Task));

    task->task_id = next_task_id++;
    strncpy(task->name, name, 31);
    task->name[31] = '\0';
    task->state = TASK_READY;

    // CFS initialization
    task->weight = nice_to_weight(nice);
    task->vruntime = PlaceNewTask();  // Smart placement to avoid starvation
    task->sum_exec_runtime = 0;
    task->exec_start = 0;
    task->min_granularity = MIN_GRANULARITY;
    task->on_rq = false;  // Will be added to run queue below

    // Allocate kernel stack for this task
    task->kernel_stack = (uint64_t)g_PMM.RequestPage();
    task->kernel_stack_size = 4096;

    // Map critical structures for user tasks
    bool isUserTask = (pml4 != nullptr && pml4 != kernelPLM4);

    if (isUserTask) {
        PageTableManager tempManager;
        tempManager.Init(pml4);

        g_serialWriter->Print("Mapping task structures for '");
        g_serialWriter->Print(name);
        g_serialWriter->Print("'\n");

        // 1. Map the Task structure itself
        tempManager.MapMemory((void*)task, (void*)task, false);

        // 2. Map the kernel stack
        tempManager.MapMemory((void*)task->kernel_stack, (void*)task->kernel_stack, false);

        // 3. Map the current task list
        if (task_list_head != nullptr) {
            tempManager.MapMemory((void*)task_list_head, (void*)task_list_head, false);
        }

        // 4. Map current_task pointer location
        uint64_t current_task_addr = (uint64_t)&current_task;
        tempManager.MapMemory((void*)(current_task_addr & ~0xFFFULL),
                             (void*)(current_task_addr & ~0xFFFULL), false);
    }

    uint64_t kstack_top = task->kernel_stack + task->kernel_stack_size;

    if (isUserTask)
    {
        uint64_t *stack = (uint64_t *)kstack_top;

        // Setup the IRET frame
        *(--stack) = 0x23;                   // SS (User Data)
        *(--stack) = (uint64_t)userStackTop; // RSP
        *(--stack) = 0x202;                  // RFLAGS (IF bit set)
        *(--stack) = 0x1B;                   // CS (User Code)
        *(--stack) = (uint64_t)entryPoint;   // RIP

        // Setup the switch_task return address
        *(--stack) = (uint64_t)user_task_trampoline;

        task->context.rsp = (uint64_t)stack;
    }
    else
    {
        // Kernel task: push entryPoint for the 'ret' in switch_task
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

    // Add to run queue
    EnqueueTask(task);

    g_screenwriter->Print("Scheduler: Created task '");
    g_screenwriter->Print(name);
    g_screenwriter->Print("' (nice=");
    char nice_str[8];
    itoa(nice, nice_str, 10);
    g_screenwriter->Print(nice_str);
    g_screenwriter->Print(", weight=");
    itoa(task->weight, nice_str, 10);
    g_screenwriter->Print(nice_str);
    g_screenwriter->Print(")\n\r");

    return task;
}

uint64_t Scheduler::PlaceNewTask()
{
    // Find the minimum vruntime among all runnable tasks
    // New task starts slightly behind to avoid unfair advantage
    uint64_t min_vruntime = 0;
    bool found = false;

    Task *task = task_list_head;
    if (task != nullptr) {
        do {
            if (task->on_rq && (task->state == TASK_READY || task->state == TASK_RUNNING)) {
                if (!found || task->vruntime < min_vruntime) {
                    min_vruntime = task->vruntime;
                    found = true;
                }
            }
            task = task->next;
        } while (task != task_list_head);
    }

    // Place new task at min_vruntime (fair start)
    return min_vruntime;
}

Task *Scheduler::SelectNextTask()
{
    // CFS: Select task with minimum vruntime
    Task *best_task = nullptr;
    uint64_t min_vruntime = UINT64_MAX;

    Task *task = task_list_head;
    if (task == nullptr) return nullptr;

    do {
        // Only consider tasks on the run queue
        if (task->on_rq && (task->state == TASK_READY || task->state == TASK_RUNNING)) {
            if (task->vruntime < min_vruntime) {
                min_vruntime = task->vruntime;
                best_task = task;
            }
        }
        task = task->next;
    } while (task != task_list_head);

    return best_task;
}

bool Scheduler::ShouldPreempt(Task *current, Task *candidate)
{
    if (current == nullptr || candidate == nullptr) return false;
    if (current == candidate) return false;

    // Calculate vruntime difference
    int64_t delta = (int64_t)candidate->vruntime - (int64_t)current->vruntime;

    // Preempt if candidate is sufficiently behind in vruntime
    // This implements the "wakeup preemption" feature
    if (delta < -(int64_t)WAKEUP_GRANULARITY) {
        return true;
    }

    return false;
}

uint64_t Scheduler::CalculateTimeSlice(Task *task)
{
    // Calculate proportional time slice based on weight
    // More weight = larger time slice

    // Total weight of all runnable tasks
    uint64_t total_weight = 0;
    Task *t = task_list_head;
    if (t != nullptr) {
        do {
            if (t->on_rq && (t->state == TASK_READY || t->state == TASK_RUNNING)) {
                total_weight += t->weight;
            }
            t = t->next;
        } while (t != task_list_head);
    }

    if (total_weight == 0) return MIN_GRANULARITY;

    // Time slice = (task_weight / total_weight) * TARGET_LATENCY
    uint64_t time_slice = (task->weight * TARGET_LATENCY) / total_weight;

    // Enforce minimum granularity to prevent thrashing
    if (time_slice < MIN_GRANULARITY) {
        time_slice = MIN_GRANULARITY;
    }

    return time_slice;
}

void Scheduler::UpdateCurrent()
{
    if (current_task == nullptr) return;

    // Calculate how long the task has been running
    uint64_t now = system_ticks;
    uint64_t delta_exec = now - current_task->exec_start;

    if (delta_exec == 0) return;

    // Update total runtime
    current_task->sum_exec_runtime += delta_exec;

    // Update vruntime: vruntime += delta_exec * (NICE_0_WEIGHT / weight)
    // Higher weight = slower vruntime growth = more CPU time
    // NICE_0_WEIGHT = 1024
    uint64_t delta_vruntime = (delta_exec * 1024) / current_task->weight;
    current_task->vruntime += delta_vruntime;

    // Reset execution start for next period
    current_task->exec_start = now;
}

void Scheduler::Schedule()
{
    if (current_task == nullptr) return;

    // Update current task's runtime statistics
    UpdateCurrent();

    // Select next task based on CFS algorithm
    Task *next_task = SelectNextTask();

    if (next_task == nullptr) {
        // No runnable tasks, keep current (should be idle)
        return;
    }

    // Check if we should preempt current task
    if (next_task != current_task) {
        // Different task selected - switch
        SwitchToTask(next_task);
    } else {
        // Same task, but check if it's used its time slice
        uint64_t time_slice = CalculateTimeSlice(current_task);
        uint64_t delta_exec = system_ticks - current_task->exec_start;

        if (delta_exec >= time_slice) {
            // Time slice expired, but same task is still best choice
            // Reset exec_start for next time slice
            current_task->exec_start = system_ticks;
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
    next_task->exec_start = system_ticks;  // Mark when this task started running
    current_task = next_task;

    g_tss.rsp0 = (next_task->kernel_stack + next_task->kernel_stack_size) & ~0xF;

    switch_task(&old_task->context, &next_task->context, next_task->page_table);
}

void Scheduler::EnqueueTask(Task *task)
{
    if (task == nullptr || task->on_rq) return;

    task->on_rq = true;
    task->state = TASK_READY;

    // In a real CFS, tasks would be inserted into a red-black tree
    // For now, we're using the circular linked list
    // The SelectNextTask() will find the minimum vruntime
}

void Scheduler::DequeueTask(Task *task)
{
    if (task == nullptr || !task->on_rq) return;

    task->on_rq = false;
}

void Scheduler::Yield()
{
    if (current_task) {
        // Update current task statistics
        UpdateCurrent();

        // Voluntarily give up CPU
        // In CFS, yielding can penalize the task slightly
        current_task->vruntime += MIN_GRANULARITY;

        Schedule();
    }
}

void Scheduler::BlockTask()
{
    if (current_task == nullptr) return;

    // Update statistics before blocking
    UpdateCurrent();

    // Remove from run queue
    DequeueTask(current_task);
    current_task->state = TASK_BLOCKED;
    current_task->sleep_start = system_ticks;

    // Force a reschedule
    Schedule();
}

void Scheduler::WakeTask(Task *task)
{
    if (task == nullptr || task->state != TASK_BLOCKED) return;

    // Calculate sleep time
    uint64_t sleep_time = system_ticks - task->sleep_start;

    // CFS sleeper fairness: tasks that slept get some vruntime credit
    // This makes interactive tasks more responsive
    // Subtract half the sleep time from vruntime (capped at minimum)
    uint64_t min_vruntime = 0;
    bool found = false;

    Task *t = task_list_head;
    if (t != nullptr) {
        do {
            if (t->on_rq && (t->state == TASK_READY || t->state == TASK_RUNNING)) {
                if (!found || t->vruntime < min_vruntime) {
                    min_vruntime = t->vruntime;
                    found = true;
                }
            }
            t = t->next;
        } while (t != task_list_head);
    }

    // Give waking task a slight advantage (helps interactive tasks)
    uint64_t credit = sleep_time / 2;
    if (task->vruntime > credit) {
        task->vruntime -= credit;
    } else {
        task->vruntime = 0;
    }

    // Don't let it go too far below minimum
    if (found && task->vruntime < min_vruntime - WAKEUP_GRANULARITY) {
        task->vruntime = min_vruntime - WAKEUP_GRANULARITY;
    }

    // Add back to run queue
    EnqueueTask(task);

    // Check if we should preempt current task
    if (current_task && ShouldPreempt(current_task, task)) {
        Yield();
    }
}

void Scheduler::SetTaskNice(Task *task, int nice)
{
    if (task == nullptr) return;

    task->weight = nice_to_weight(nice);

    g_serialWriter->Print("Task '");
    g_serialWriter->Print(task->name);
    g_serialWriter->Print("' nice changed to ");
    char nice_str[8];
    itoa(nice, nice_str, 10);
    g_serialWriter->Print(nice_str);
    g_serialWriter->Print(" (weight=");
    itoa(task->weight, nice_str, 10);
    g_serialWriter->Print(nice_str);
    g_serialWriter->Print(")\n");
}

Task *Scheduler::GetCurrentTask()
{
    return current_task;
}

void Scheduler::ExitTask()
{
    if (current_task)
    {
        DequeueTask(current_task);
        g_screenwriter->Print("Exiting task with id: ");
        g_screenwriter->PrintDecimalPadded(current_task->task_id,10);
        g_screenwriter->Print("\n");
        current_task->state = TASK_TERMINATED;
        Yield();
    }
}
