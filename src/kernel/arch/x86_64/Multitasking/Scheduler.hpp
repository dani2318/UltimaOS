#pragma once
#include <arch/x86_64/Multitasking/Task.hpp>

class Scheduler {
public:
    static void Initialize();
    static Task* CreateTask(void (*entryPoint)(), const char* name, int priority, PageTable* pml4, void* arg = nullptr);
    static void Schedule();  // Called from timer interrupt
    static void Yield();     // Voluntarily give up CPU
    static Task* GetCurrentTask();
    static void ExitTask();
    
private:
    static Task* current_task;
    static Task* task_list_head;
    static uint64_t next_task_id;
    
    static Task* SelectNextTask();
    void PrepareShellTask(uint64_t entryPoint, void* apiTable);
    static void SwitchToTask(Task* next_task);
};