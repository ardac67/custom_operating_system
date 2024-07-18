//tekli fork yapabiliyorsun
//tasklara parametre paslayamıyorsun
//init processde create etmen lazım onuda daha yapmadın





#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>


// #define GRAPHICSMODE


using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;

GlobalDescriptorTable gdt;
TaskManager taskManager(&gdt);

void printf(char* str)
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


void printInt(int digit) 
{
    char buff[256];
    int n; 
    int i;
    
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



void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}





class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char* foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;
public:
    
    MouseToConsole()
    {
        uint16_t* VideoMemory = (uint16_t*)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);        
    }
    
    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);

        x += xoffset;
        if(x >= 80) x = 79;
        if(x < 0) x = 0;
        y += yoffset;
        if(y >= 25) y = 24;
        if(y < 0) y = 0;

        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);
    }
    
};




void sysprintf(char* str)
{
    asm("int $0x80" : : "a" (4), "b" (str));
}

// System Call Fork:
// puts the syscall 1 to eax register and triggers the interrupt 0x80
// returns the result of the syscall according to the eax register
// returns value as a child pid for further use
int fork() {
    int result;
    asm("int $0x80" : "=a" (result) : "a" (1));
    return result;
}

//System Call Execve:
// puts the syscall 2 to eax register and triggers the interrupt 0x80
// returns the result of the syscall according to the eax register which is the entrypoint
// and the parameter of the entrypoint (there exists here but not used i just wanted to show)
void execve(void entrypoint(int) ,int param) {
    asm("int $0x80" : : "a" (2),"b" (entrypoint),"c" (param));
}

//System Call Waitpid:
//gets pid parameter that refers to child should be waited
// puts the syscall 3 to eax register and triggers the interrupt 0x80 
void waitpid(int pid) {
    asm("int $0x80" : : "a" (3),"b" (pid));
}
//System Call Exit:
// puts the syscall 5 to eax register and triggers the interrupt 0x80
// exists the current executing process which simply puts EXITED into task status
void exit(){
    asm("int $0x80" : : "a" (5));
}

//Task should be implemented according to the pdf
void collatz(int n)
{   
    n = 6;
    int buffer[100];  
    int i = 0;
    buffer[i] = n;

    while (n != 1 && i < 100) { 
        if (n % 2 == 0) {
            n = n / 2;
        } else {
            n = 3 * n + 1;
        }
        buffer[++i] = n;
    }

    printf("Collatz\n");
    for (int j = 0; j <= i; j++) {
        if(buffer[j]!=1); printInt(buffer[j]);
    } 
    printf("\n");
    exit();  
}
void long_running_program(int a) {
    int n =100;
    int result = 0;
    printf("Long Running Program\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            result += i * j;
        }
    }
    printf("Result: ");
    printInt(result);
    printf("\n");
    exit();
}


//Parent processes that executes childs and waits until them finish
//When they are waiting in waitpid syscall puts them BLOCKED state
void taskA()
{
    int a = fork(); 
    if(a == 0)//child executes this part
    {
        execve(collatz,6); //child executes collatz function   
    }
    else
    {
        printf("Task A started\n");
        waitpid(a);//parent waits until child finishes
        printf("Task A finished\n");
        exit();//then exists
    }
}

void taskB()
{
    int a = fork(); 
    if(a == 0)//child executes this part
    {
        execve(long_running_program,6);//child executes long_running_program function 
    }
    else
    {
        printf("Task B started\n");
        waitpid(a);//parent waits until child finishes
        printf("Task B finished\n");
        exit();//then exists
    }
        
}

//Init process always runs in the system
void init(){
    while(true){
        //taskManager.incrementTick();
    }
}

void init_process(TaskManager *taskManager, GlobalDescriptorTable *gdt){

    Task *init_task = new Task(gdt, init, taskManager);
    //adding init task to task manager
    taskManager->AddTask(init_task);
    Task *task1,*task2;
    for(int i=0;i<3;i++){
        //adding parent tasks to task manager
        task1 = new Task(gdt, taskA, taskManager);
        task2 = new Task(gdt, taskB, taskManager);
        taskManager->AddTask(task1);
        taskManager->AddTask(task2); 
    }
}

void initialize_first_process(){
    init_process(&taskManager,&gdt);
}


typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}



extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello from my OS!\n");
   
    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
    size_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
    
    
    void* allocated = memoryManager.malloc(1024);
    initialize_first_process();

    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&taskManager,&interrupts, 0x80);
    interrupts.Activate();

    while(1)
    {
        //taskManager.incrementTick();
    }
}

