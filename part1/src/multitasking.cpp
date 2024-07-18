
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

void printf(char *str);
void printInt(int num);


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

Task::Task(GlobalDescriptorTable *gdt, void entrypoint(),TaskManager *taskManager)
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


    //defining taskState ready because it is created newly
    taskState = READY;   
    //not used field
    time_process_started = 0;
}
Task::Task(){}

//unnecessary constructor i dont remember why i have created it.
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
    // cpustate -> ss = ;
    cpustate -> eflags = 0x202;
    //printf("Task state in task constructor: ");
    //printf("READY\n");
    taskState = READY;
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
TaskManager::TaskManager(GlobalDescriptorTable *gdt)
{
    //Number of Tasks
    numTasks = 0;
    //Currently Executed Task
    currentTask = -1;
    //PID Counter
    current_pid = 0;
    this->gdt = gdt;
    //Not Used
    tick = 0;
}

TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task* task)
{
    if(numTasks >= 256)
        return false;

    //Setting task state ready when adding it to task table
    task->set_task_state(READY);
    task->setTimeStart(getTick());
    //incrementing number of tasks

    tasks[numTasks++] = task;
    //Setting it's pid according to the last used pid
    task->set_pid( get_last_pid() );
    //Setting it's parent pid to 0 because it is not created with fork, it's parent should be init
    task->set_parent_pid(0);

    return true;
}

void TaskManager::do_scheduling(){

}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{   
    //If there is no tasks no need to change cpustate return as it is
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask]->cpustate = cpustate;
    

    //If current task is running set it's state to ready
    //Because timer interrupt occurs it should give to another process a chance to execute itself
    if (tasks[currentTask]->get_task_state() == RUNNING){
        tasks[currentTask]->set_task_state(READY);
    }

    do { 
        //Apply modulus to get the next task like an circler array as long as it is READY state
        if (++currentTask >= numTasks)
            currentTask %= numTasks;
    } while (tasks[currentTask]->taskState != READY);
    
    //Set the selected task as RUNNING
    tasks[currentTask]->taskState = RUNNING;

    //Return the new cpustate state as selected task's cpustate
    return tasks[currentTask]->cpustate;
}

int TaskManager::get_last_pid()
{
    current_pid = current_pid + 1;
    return current_pid;
}

//Used to managing fork system call 
//Takes the cpustate as parameter
int TaskManager::fork(CPUState* cpustate)
{
    //Getting the currently executed task which will fork itself
    Task* parent = tasks[currentTask];

    //If process is parent process it can fork itself
    //If it is not parent process it can't fork itself
    //This prevents childs forking themselves infinitely
    if(parent->get_parent_pid() == 0)
    {
        //Creating empty tasks for child
        Task* child = new Task(gdt, 0, this);
        //Copying the parent's cpustate to child's cpustate
        *(child->cpustate) = *cpustate;
        //Copying parent instruction pointer to child instruction pointer
        child->cpustate->eip = parent->cpustate->eip;
        //Setting child pid according to last one used
        child->set_pid( get_last_pid() );
        //Setting parent pid to parent's pid
        child->set_parent_pid( parent->get_pid() );
        //Setting child task state to ready 
        child->set_task_state(READY);
        //Not used
        child->setTimeStart(this->getTick());
        //Finally adding to task table
        tasks[numTasks] = child;
        ++numTasks;
        //Returning child's pid to further use by parent
        return child->get_pid();
    }
    else{
        //If it is not parent process return 0
        return 0;
    } 
} 

//Changes current process image with the provided entrypoint(eip), param and cpustate
common::uint32_t TaskManager::execve(void (*entrypoint)(),int param,CPUState* cpustate) {
    //Gets the current task and replaced it with the newly created task but sending pid and parent pid and process start time
    tasks[currentTask]->setTaskForExec(gdt,entrypoint, tasks[currentTask]->get_parent_pid(),tasks[currentTask]->get_pid(), tasks[currentTask]->time_process_started );
    //Returns the new cpustate of current task with core image changed
    return (uint32_t)tasks[currentTask]->cpustate;
}

//Simply helper function to execv syscall which implements the fresh start of process which does execv syscall
void Task::setTaskForExec(GlobalDescriptorTable *gdt, void entrypoint(),common::uint32_t parent_pid,common::uint32_t pid,int process_start)
{
    //Sets instruction pointer of the task to the entrypoint
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    cpustate -> eflags = 0x202;
    //Setting parent pid as old one has
    this->parent_pid = parent_pid;
    //Setting pid as old one has
    this->pid = pid;
    this->time_process_started = process_start;
    //Setting the task state to READY for further scheduling
    taskState = READY;   
}


//Waits until the child process with the given pid finishes
//Also returns the schedule new state
common::uint32_t TaskManager::waitpid(int pid,CPUState* cpustate) {
    //Getting the current task
    Task* current_process = tasks[currentTask];
    //Loop through as long as if you find your child and if it is exited return the new scheduler state with setting itself ready
    //If child not exited BLOCKED itself and return the new scheduler state
    while (true) {
        bool found = false;
        for (int i = 0; i < numTasks; i++) {
            if (tasks[i]->get_pid() == pid && tasks[i]->get_parent_pid() == current_process->get_pid()) {
                found = true;
                if (tasks[i]->get_task_state() == EXITED) {
                    current_process->set_task_state(READY);
                    //Returning new cpu state
                    return (uint32_t)Schedule(cpustate);
                }
                else{
                    current_process->set_task_state(BLOCKED);
                    //Returning new cpu state
                    return (uint32_t)Schedule(cpustate);
                }
            }
        }
        if (!found){
            //If child somehow exited return the new scheduler state with setting itself ready
            //OR maybe you dont have a child so you do not need to wait for it
            current_process->set_task_state(READY);
            return (uint32_t)Schedule(cpustate); 
        } 
    }
} 

//Implements the exit syscall in taskManager class
//Sets the current process exit situtation and sets it's state to EXITED
common::uint32_t TaskManager::exit(CPUState* cpustate) {
    //Sets directly state exited
    tasks[currentTask]->set_task_state(EXITED);
    //sets cpu state all zeros to somehow cleaning the memory
    tasks[currentTask]->cpustate = 0;
    tasks[currentTask]->process_end_time = this->getTick();

    //Loops through all tasks if it is a child and it has parent sets parent state ready and returns the new scheduler state of new process
    for(int i = 0; i < numTasks; i++){
        if( tasks[i]->get_pid() == tasks[currentTask]->get_parent_pid() ){
            tasks[i]->set_task_state(READY);
        }
    }
    //Returning new process state as cpustate
    return (uint32_t)Schedule(cpustate);
} 

//Prints process table with pid, parent pid and state
void TaskManager::PrintTaskTable() 
{
    printf("\n");    
    printf("PID   PPID   State\n");
    for (int i = 0; i < numTasks; ++i)
    {
        Task *task = tasks[i];
        printInt(task->get_pid());  printf("     "); 
        printInt(task->get_parent_pid()); printf("      ");

        switch (task->get_task_state()) {
            case READY:         printf("Ready     "); break;
            case RUNNING:       printf("Running   "); break;
            case BLOCKED:       printf("Blocked   "); break;
            case EXITED:        printf("Exited    "); break;
            default:            printf("Unknown   "); break;
        }
        printf("\n");
    }
    printf("\n");
}

void TaskManager::incrementTick()
{
    this->tick = this->tick + 1;
}

void TaskManager::setProcessTick(){
    tasks[currentTask]->setTimeStart(this->tick);
}

void Task::setTimeStart(int start)
{
    time_process_started = start;
}

int TaskManager::getTick(){
    return this->tick;
} 