 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos
{
    class TaskManager;
    enum TaskState {
        READY,   // Task is ready to run
        RUNNING, // Task is currently running
        BLOCKED,  // Task is blocked and cannot proceed
        EXITED
    };
    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;        
    } __attribute__((packed));
    
    
    class Task
    {
    friend class TaskManager;
    private:
        common::uint8_t stack[4096]; // 4 KiB
        CPUState* cpustate;
        //common::uint32_t parent_pid;
        TaskState taskState;
        int pid;
        int parent_pid;
        void* entryArgument;
        int time_process_started;
        int process_end_time;
    public:
        Task(GlobalDescriptorTable *gdt, void entrypoint(),TaskManager *taskManager);
        Task(void (*entrypoint)(void*), void* arg, GlobalDescriptorTable *gdt);
        Task();
        ~Task();
        void set_task_state(TaskState state);
        void set_pid(int pid);
        int get_pid();
        void set_parent_pid(int parent_pid);
        void setTaskForExec(GlobalDescriptorTable *gdt, void entrypoint(),common::uint32_t parent_pid,common::uint32_t pid,int process_start );
        int get_parent_pid();
        TaskState get_task_state();
        int argument;
        void setTimeStart(int start);
    };
    
    
    class TaskManager
    {
    private:
        Task* tasks[256];
        int numTasks;
        int currentTask;
        int current_pid;
        GlobalDescriptorTable *gdt;
        int tick;
    public:
        TaskManager(GlobalDescriptorTable *gdt);
        ~TaskManager();
        bool AddTask(Task* task);
        CPUState* Schedule(CPUState* cpustate);
        int get_last_pid();
        int fork(CPUState* cpustate);
        common::uint32_t execve(void (*entrypoint)(),int param,CPUState* cpustate);
        common::uint32_t waitpid(int pid, CPUState* cpustate);
        //int waitpid(int pid);
        //anlamadım
        void do_scheduling();
        common::uint32_t exit(CPUState* cpustate);

        //printing task table
        void PrintTask(int tableIndex);
        void PrintTaskTable();
        void incrementTick();
        void setProcessTick();
        int getTick();
    };
    
    
    
}


#endif