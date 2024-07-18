
#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
void printInt(int number);
void printfTest(char* str)
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
 
SyscallHandler::SyscallHandler(TaskManager *taskManager,InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber  + interruptManager->HardwareInterruptOffset())
{
    this->taskManager = taskManager;
}

SyscallHandler::~SyscallHandler()
{
}




uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;
    

    switch(cpu->eax)
    {   
        case 1:{
            //fork function in taskmanager returns the forked child pid
            int pid = taskManager->fork(cpu);
            //putting the pid in eax register to further use in parent process
            cpu->eax = pid;
            break;
        }
        case 2:
            //Execv takes the entrypoint of the new process and it's arguments but argument not used
            //Also puts cpu state in one of the parameters
            //Returns the new esp which is new cpu state,
            esp = taskManager->execve((void (*)()) cpu->ebx,cpu->ecx,cpu);
            break;
        case 3:
            //Waitpid takes the pid of child process from ebx register and cpu state
            //returns new scheduler state
            esp = taskManager->waitpid(cpu->ebx,cpu);
            break;
        case 4:
            //just for testing syscall
            printfTest((char*)cpu->ebx);
            break;
        case 5:
            //Exits from current process
            //gets cpu state as paramater and returns new scheduler state
            //returns esp for context switching very crucial
            esp = taskManager->exit(cpu);
            break;
        default:
            break;
    }

    
    return esp;
}

