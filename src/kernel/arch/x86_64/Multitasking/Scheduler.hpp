#pragma once
#include <arch/x86_64/Multitasking/Task.hpp>

class Scheduler
{
public:
    static void Initialize();

    // Create a new task with nice value (default 0)
    // nice: -20 (highest priority) to +19 (lowest priority)
    static Task *CreateTask(void (*entryPoint)(), const char *name, int nice,
                            PageTable *pml4, void *userStackTop);

    static void Schedule(); // Called from timer interrupt
    static void Yield();    // Voluntarily give up CPU
    static Task *GetCurrentTask();
    static void ExitTask();

    // Block current task (for I/O wait, etc.)
    static void BlockTask();

    // Wake up a blocked task
    static void WakeTask(Task *task);

    // Change task's nice value (priority)
    static void SetTaskNice(Task *task, int nice);

    // Get system uptime in timer ticks
    static uint64_t GetTicks() { return system_ticks; }
    static void IncrementSystemTicks() { system_ticks++; };
private:
    static Task *current_task;
    static Task *task_list_head;
    static uint64_t next_task_id;
    static uint64_t system_ticks;  // Global tick counter

    // CFS parameters (in timer ticks)
    // Assuming 1ms timer tick (1000 Hz)
    static const uint64_t TARGET_LATENCY = 20;      // 20ms target latency
    static const uint64_t MIN_GRANULARITY = 4;      // 4ms minimum granularity
    static const uint64_t WAKEUP_GRANULARITY = 2;   // 2ms for wakeup preemption

    // Select next task to run (CFS core algorithm)
    static Task *SelectNextTask();

    // Switch to a different task
    static void SwitchToTask(Task *next_task);

    // Add task to run queue
    static void EnqueueTask(Task *task);

    // Remove task from run queue
    static void DequeueTask(Task *task);

    // Update current task's runtime statistics
    static void UpdateCurrent();

    // Check if current task should be preempted
    static bool ShouldPreempt(Task *current, Task *candidate);

    // Place new task's vruntime (avoid giving too much advantage)
    static uint64_t PlaceNewTask();

    // Calculate time slice for a task based on weight
    static uint64_t CalculateTimeSlice(Task *task);
};
