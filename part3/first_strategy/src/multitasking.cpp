
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

void printf(char *str);
void printInt(int num);
void taskA();
void taskB();
void taskC();
void taskD();

bool isValidState(TaskState state) {
    return state == READY || state == RUNNING || state == BLOCKED || state == EXITED;
}

void printfNewTest(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}
void printIntTest(int digit) 
{
    char buff[256];
    int n; 
    int i;
    
    // check if the digit is positive or negative
    if (digit < 0) {
        digit *= -1;
        buff[0] = '-';
        i = n = 1;
    }
    else {
        i = n = 0;
    }

    do {
        buff[n] = '0' + (digit % 10);
        digit /= 10;
        ++n;
    } while (digit > 0);

    buff[n] = '\0';
    
    while (i < n / 2) {
        int temp = buff[i];
        buff[i] = buff[n - i - 1];
        buff[n - i - 1] = temp;
        ++i;        
    }
    printf((char *) buff);
}

void temp_task_for_fork()
{
    while(true)
    {
        //printfNewTest("F");
    }
}

Task::Task(GlobalDescriptorTable *gdt, void entrypoint(),TaskManager *taskManager,Priority priority)
{
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;

    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    cpustate -> eflags = 0x202;
    taskState = READY;   
    time_process_started = 0;
    this->priority = priority;
}
Task::Task(){}
Task::Task(void (*entrypoint)(void*), void* arg, GlobalDescriptorTable *gdt): entryArgument(arg){

    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;
    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    cpustate -> eflags = 0x202;
    taskState = READY;
    int* intArg = static_cast<int*>(arg); 
    int value = *intArg;

}
Task::~Task()
{
}

TaskState Task::get_task_state()
{
    return this->taskState;
}
void Task::set_task_state(TaskState state)
{
    taskState = state;
}  
   
void Task::set_pid(int pid)
{
    this->pid = pid;
}
int Task::get_pid()
{
    return this->pid;
}
void Task::set_parent_pid(int parent_pid)
{
    this->parent_pid = parent_pid;
}
int Task::get_parent_pid()
{
    return this->parent_pid;
}
void Task::set_priority(Priority priority)
{
    this->priority = priority;
}
Priority Task::get_priority()
{
    return this->priority;
}
TaskManager::TaskManager(GlobalDescriptorTable *gdt)
{
    for (int i = 0; i < 4; ++i) {
        taskCount[i] = 0;
        for (int j = 0; j < 20; ++j) {
            tasks[i][j] = nullptr;
        }
    }
    currentTask = -1;
    current_pid = 0;
    this->gdt = gdt;
    tick = 0;
    interrupt_count = 0;
}

TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task* task)
{
    task->set_task_state(READY);
    task->setTimeStart(getTick());
    tasks[task->get_priority()][taskCount[task->get_priority()]++] = task;
    task->set_pid( get_last_pid() );
    task->set_parent_pid(0);
    return true;
}
void TaskManager::do_scheduling() {
    int bestPriority = -1;
    int bestIndex = -1;
    bool isHigherPriorityFound = false;
    for (int priority = 4; priority > currentTaskPriority; priority--) {
        for (int i = 0; i < taskCount[priority]; i++) {
            if (tasks[priority][i]->get_task_state() == READY) {
                bestPriority = priority;
                bestIndex = i;
                isHigherPriorityFound = true;
                break;
            }
        }
        if (isHigherPriorityFound) break;
    }
    int found_that = 0;
    if (!isHigherPriorityFound && currentTaskPriority != -1) {
        for (int i = 0; i < taskCount[currentTaskPriority]; i++) {
            if (tasks[currentTaskPriority][i]->get_task_state() == READY && i != currentTaskIndex) {
                found_that = 1;
                bestPriority = currentTaskPriority;
                bestIndex = i;
                break;
            }
        }
    }
    int found_that2 = 0;
    if(!found_that && currentTaskPriority != -1){
        for (int i = 0; i < taskCount[currentTaskPriority]; i++) {
            if (tasks[currentTaskPriority][i]->get_task_state() == RUNNING) {
                found_that2 = 1;
                bestPriority = currentTaskPriority;
                bestIndex = i;
                break;
            }
        }
        if(!found_that2 && currentTaskPriority != -1 ) currentTaskPriority = currentTaskPriority -1;
    }
    if (bestPriority != -1) {
        if (currentTaskPriority != -1 && tasks[currentTaskPriority][currentTaskIndex]->get_task_state() == RUNNING) {
            tasks[currentTaskPriority][currentTaskIndex]->set_task_state(READY);
        }
        currentTaskPriority = bestPriority;
        currentTaskIndex = bestIndex;
        tasks[currentTaskPriority][currentTaskIndex]->set_task_state(RUNNING);
    }
}

int TaskManager::get_interrupt_count(){
    return interrupt_count;
}

CPUState* TaskManager::Schedule(CPUState* cpustate) {

    interrupt_count = interrupt_count + 1;
    if (currentTaskPriority != -1 && currentTaskIndex != -1) {
        // Save the current CPU state back to the task structure
        tasks[currentTaskPriority][currentTaskIndex]->cpustate = cpustate;
    }

    do_scheduling();
    if (currentTaskPriority != -1 && currentTaskIndex != -1) {
        return tasks[currentTaskPriority][currentTaskIndex]->cpustate;
    }
    return cpustate;
}

int TaskManager::get_last_pid()
{
    current_pid = current_pid + 1;
    return current_pid;
}
int TaskManager::fork(CPUState* cpustate)
{
    if (currentTaskPriority == -1 || currentTaskIndex == -1 ||
        currentTaskPriority >= 4 || currentTaskIndex >= taskCount[currentTaskPriority]) {
        return 0;  // No valid current task
    }

    Task* parent = tasks[currentTaskPriority][currentTaskIndex];
    if (parent && parent->get_parent_pid() == 0) { 
        if (taskCount[currentTaskPriority] >= 20) {
            return 0; 
        }
        Task* child = new Task(gdt, 0, this, parent->get_priority());
        *(child->cpustate) = *cpustate;
        child->cpustate->eip = parent->cpustate->eip;
        child->set_pid(get_last_pid());
        child->set_parent_pid(parent->get_pid());
        child->set_task_state(READY);
        child->setTimeStart(this->getTick());
        child->priority = parent->priority;
        tasks[currentTaskPriority][taskCount[currentTaskPriority]] = child;
        taskCount[currentTaskPriority]++;

        return child->get_pid();
    } else {
        return 0;
    }
}
void TaskManager::BlockedToReady(){
    int found = 0;
    for (int priority = 4; priority > currentTaskPriority; priority--) {
        for (int i = 0; i < taskCount[priority]; i++) {
            if (tasks[priority][i]->get_task_state() == BLOCKED && tasks[priority][i]->get_parent_pid() !=0) {
                tasks[priority][i]->set_task_state(READY);
                found = 1;
                break;
            }
            if(found){
                break;
            }
        }
        if(found){
            break;
        }
    }
}

int TaskManager::get_keyboard_event(CPUState* cpustate)
{
    
} 
common::uint32_t TaskManager::execve(void (*entrypoint)(), int param, CPUState* cpustate) {
    Task* currentTask = tasks[currentTaskPriority][currentTaskIndex];
    currentTask->setTaskForExec(gdt, entrypoint, currentTask->get_parent_pid(), currentTask->get_pid(), currentTask->time_process_started);
    return (common::uint32_t) currentTask->cpustate;
}

void Task::setTaskForExec(GlobalDescriptorTable *gdt, void entrypoint(),common::uint32_t parent_pid,common::uint32_t pid,int process_start)
{
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    cpustate -> eflags = 0x202;
    this->parent_pid = parent_pid;
    this->pid = pid;
    this->time_process_started = process_start;
    taskState = READY;   
}

common::uint32_t TaskManager::waitpid(int pid, CPUState* cpustate) {
    if (currentTaskPriority == -1 || currentTaskIndex == -1 ||
        currentTaskPriority >= 4 || currentTaskIndex >= taskCount[currentTaskPriority]) {
        return (common::uint32_t)cpustate;
    }

    Task* current_process = tasks[currentTaskPriority][currentTaskIndex];
    while (true) {
        bool found = false;
        Task* target_task = nullptr;
        for (int priority = 0; priority < 4; ++priority) {
            for (int i = 0; i < taskCount[priority]; ++i) {
                if (tasks[priority][i]->get_pid() == pid && tasks[priority][i]->get_parent_pid() == current_process->get_pid()) {
                    found = true;
                    target_task = tasks[priority][i];
                    break;
                }
            }
            if (found) break;
        }

        if (!found) {
            current_process->set_task_state(READY);
            return (common::uint32_t)Schedule(cpustate);
        }
        if (target_task->get_task_state() == EXITED) {
            current_process->set_task_state(READY);
            return (common::uint32_t)Schedule(cpustate);
        } else {
            current_process->set_task_state(BLOCKED);
            return (common::uint32_t)Schedule(cpustate);
        }
    }
}


common::uint32_t TaskManager::exit(CPUState* cpustate) {
    if (currentTaskPriority < 0 || currentTaskIndex < 0 ||
        currentTaskPriority >= 4 || currentTaskIndex >= taskCount[currentTaskPriority]) {
        return (common::uint32_t)cpustate;
    }
    Task* exitingTask = tasks[currentTaskPriority][currentTaskIndex];
    exitingTask->set_task_state(EXITED);
    for (int priority = 0; priority < 4; ++priority) {
        for (int i = 0; i < taskCount[priority]; ++i) {
            if (tasks[priority][i]->get_pid() == exitingTask->get_parent_pid()) {
                tasks[priority][i]->set_task_state(READY);
            }
        }
    }
    return (common::uint32_t)Schedule(cpustate);
}


void TaskManager::PrintTaskTable() 
{
    printf("\n");
    printf("PID   PPID   State Priority\n");

    for (int priority = 0; priority < 4; ++priority) {
        for (int i = 0; i < taskCount[priority]; ++i) {
            Task *task = tasks[priority][i];
            printInt(task->get_pid());  printf("     ");
            printInt(task->get_parent_pid()); printf("      ");
            switch (task->get_task_state()) {
                case READY:   printf("Ready     "); break;
                case RUNNING: printf("Running   "); break;
                case BLOCKED: printf("Blocked   "); break;
                case EXITED:  printf("Exited    "); break;
                default:      printf("Unknown   "); break;
            }
         
            switch (priority) {
                case 0: printf("VeryLow    "); break;
                case 1: printf("Low        "); break;
                case 2: printf("Medium     "); break;
                case 3: printf("High       "); break;
                default: printf("Unknown    "); break;
            }
            printf("    ");
            printf("\n");
        }
    }
    printf("\n");
}


void TaskManager::incrementTick()
{
    this->tick = this->tick + 1;
}

void TaskManager::setProcessTick(){
    Task* exitingTask = tasks[currentTaskPriority][currentTaskIndex];
    exitingTask->setTimeStart(this->tick);
}

void Task::setTimeStart(int start)
{
    time_process_started = start;
}

int TaskManager::getTick(){
    return this->tick;
} 