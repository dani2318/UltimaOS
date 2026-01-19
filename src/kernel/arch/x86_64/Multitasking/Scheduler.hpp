#pragma once
#include <arch/x86_64/Multitasking/Task.hpp>
#include <globals.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>
#include <cpp/String.hpp>
#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP
// class Scheduler
// {
// public:
//     static void Initialize(PageTableManager& ptm);
//     static Task *CreateTask(void (*entryPoint)(), const char *name, int priority,
//                                 PageTable *pml4, void *userStackTop);
//     static void Schedule(); // Called from timer interrupt
//     static void Yield();    // Voluntarily give up CPU
//     static Task *GetCurrentTask();
//     static void ExitTask();

// private:
//     static Task *current_task;
//     static Task *task_list_head;
//     static uint64_t next_task_id;

//     static Task *SelectNextTask();
//     void PrepareShellTask(uint64_t entryPoint, void *apiTable);
//     static void SwitchToTask(Task *next_task);
// };

class Scheduler {
    private:
        Task* readyQueueHead;
        Task* currentTask;
        uint64_t nextTaskID;

        void addToReadyQueue(Task* task);
        void removeFromReadyQueue(Task* task);
        Task* findTask(uint64_t task_id);
        Task* getNextTask();
    public:
        Scheduler();
        Task* CreateTask(const char* name, void (*entry_point)(), uint64_t priority);
        void DestroyTask(uint64_t taskID);

        void Schedule();
        void Yield();
        void BlockCurrentTask();
        void UnblockTask(uint64_t taskID);

        void timerTick();
};
#endif