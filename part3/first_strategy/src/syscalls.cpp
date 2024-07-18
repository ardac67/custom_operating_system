
#include <syscalls.h>
#include <drivers/keyboard.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
void printInt(int number);
void printf(char* str);
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
 
SyscallHandler::SyscallHandler(TaskManager *taskManager,InterruptManager* interruptManager, uint8_t InterruptNumber,myos::drivers::KeyboardDriver* keyboard)
:    InterruptHandler(interruptManager, InterruptNumber  + interruptManager->HardwareInterruptOffset())
{
    this->taskManager = taskManager;
    this->keyboard = keyboard;
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
            int pid = taskManager->fork(cpu);
            cpu->eax = pid;
            break;
        }
        case 2:
            esp = taskManager->execve((void (*)()) cpu->ebx,cpu->ecx,cpu);
            break;
        case 3:
            esp = taskManager->waitpid(cpu->ebx,cpu);
            break;
        case 4:
            printfTest((char*)cpu->ebx);
            break;
        case 5:
            esp = taskManager->exit(cpu);
            break;
        case 6:{
            //gets last pressed key
            common::int32_t return_value = keyboard->GetLastKey();
            //clears the key field in keyboard class
            keyboard->ClearLastKey();
            //puts that return value to the eax register
            cpu->eax = return_value;
            break;
        }
        default:
            break;
    }

    
    return esp;
}

