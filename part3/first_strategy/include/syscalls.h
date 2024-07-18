 
#ifndef __MYOS__SYSCALLS_H
#define __MYOS__SYSCALLS_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <multitasking.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>


namespace myos
{
    
    class SyscallHandler : public hardwarecommunication::InterruptHandler
    {
        
    public:
        SyscallHandler(TaskManager *taskManager,hardwarecommunication::InterruptManager* interruptManager, myos::common::uint8_t InterruptNumber,myos::drivers::KeyboardDriver* keyboard);
        ~SyscallHandler();
        
        virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);
    private:
        TaskManager* taskManager;
        myos::drivers::KeyboardDriver* keyboard;
    };
    
    
}


#endif