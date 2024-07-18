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

int32_t random(int min, int max) 
{
    uint64_t counter;
    int32_t num;
    asm("rdtsc": "=A"(counter));
    counter = counter * 1103515245 + 12345;
    num = (int)(counter / 65536) % (max - min);
    if (num < 0)
        num += max;
    return num + min;
}



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
int fork() {
    int result;
    asm("int $0x80" : "=a" (result) : "a" (1));
    return result;
}

void execve(void entrypoint(int) ,int param) {
    asm("int $0x80" : : "a" (2),"b" (entrypoint),"c" (param));
}
void waitpid(int pid) {
    asm("int $0x80" : : "a" (3),"b" (pid));
}
void exit(){
    asm("int $0x80" : : "a" (5));
}
void collatz(int n)
{   
    n = 1000000000;
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

void binary_search(int){
    printf("Binary Search\n");
    int low = 0;
    int x = 8;
    int arr[10] = {1,2,3,4,5,6,7,8,9,10};
    int high = 10 - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == x) {
            printInt(mid);
        }
        if (arr[mid] < x) {
            low = mid + 1;
        }
        else {
            high = mid - 1;
        }
    }

    exit();
}
void linear_search(int){
    printf("Linear Search\n");
    int array[10] = {1121,4124,3333,12,123,662,8,8,99,67};
    int target  = 99;
    for(int i=0;i<10;i++){
        if(array[i] == target){
            printf("Target found at index: ");
            printInt(i);
            printf("\n");
            break;
        }
    }
    exit();
}

void test_prog(){
    while(true);
}
void taskA()//collatz
{
    int a = fork(); 
    if(a == 0)
    {
        execve(collatz,10000000000000);  
    }
    else
    {
        waitpid(a);
        exit();
    } 
}

void taskB()
{
    int a = fork(); 
    if(a == 0)
    {
        execve(long_running_program,6); 
    }
    else
    {
        waitpid(a);
        exit();
    }
        
}
void taskC()//binary_search
{
    int a = fork(); 
    if(a == 0)
    {
        execve(binary_search,6); 

    }
    else
    {
        waitpid(a);
        exit();
    }
        
}
void taskD()//linear_search
{
    int a = fork(); 
    if(a == 0)
    {
        execve(linear_search,6); 
    }
    else
    {
        waitpid(a);
        exit();
    }
        
}
void init(){
    while(true){
        exit();
    }
}
void init2(){
    printf("init2 starts\n");
    while(true){
        exit();
    }
}
//implements first strategy
//randomly chosess programs and creates 10 tasks and it's childs
//5th task is created with taskD showing the priority of the tasks works
void firstStrategy(TaskManager *taskManager, GlobalDescriptorTable *gdt){
    int rand = random(0,3);
    void (*programs[4]) (void) = {taskA,taskB ,taskC ,taskD};
    Task *task;
    void (*entrypoint)(void) = programs[rand];
    for(int i=0;i<9;i++){
        task = new Task(gdt, entrypoint, taskManager,MEDIUM);
        taskManager->AddTask(task);
         if(i == 5){//just for showing differences of priorties
            task = new Task(gdt, taskD, taskManager,LOW);
            taskManager->AddTask(task);
        }  
    }
}

//implements second strategy
//randomly chosess programs and creates 3 tasks and it's childs
void second_strategy(TaskManager *taskManager, GlobalDescriptorTable *gdt){
    int rand1 = random(0,3);
    int rand2 = random(0,3);
    void (*programs[4]) (void) = {taskA,taskB ,taskC ,taskD};
    Task *task1,*task2,*task;
    void (*entrypoint1)(void) = programs[rand1];
    void (*entrypoint2)(void) = programs[rand2];
    for(int i=0;i<3;i++){
        task1 = new Task(gdt, entrypoint1, taskManager,MEDIUM);
        task2 = new Task(gdt, entrypoint2, taskManager,HIGH);
        taskManager->AddTask(task1);
        taskManager->AddTask(task2);
        if(i == 5){ //just for indicating differences of priorities
            task = new Task(gdt, init2, taskManager,LOW);
            taskManager->AddTask(task);
        } 
    }

}

//after 5th interrupt same priority tasks are created in TaskManager schedule function
void third_strategy(TaskManager *taskManager, GlobalDescriptorTable *gdt){
    Task * collatz = new Task(gdt, taskA, taskManager,MEDIUM);
    taskManager->AddTask(collatz);
}

//not implemented
void fourth_strategy(TaskManager *taskManager, GlobalDescriptorTable *gdt){
    Task *task1 = new Task(gdt, taskA, taskManager,LOW);
    Task *task2 = new Task(gdt, taskB, taskManager,HIGH);
    Task *task3 = new Task(gdt, taskC, taskManager,HIGH);
    Task *task4 = new Task(gdt, taskD, taskManager,HIGH);
    taskManager->AddTask(task1);
    taskManager->AddTask(task2);
    taskManager->AddTask(task3);
    taskManager->AddTask(task4);
}
void init_process(TaskManager *taskManager, GlobalDescriptorTable *gdt){

    Task *init_task = new Task(gdt, init, taskManager,HIGH);
    taskManager->AddTask(init_task);

    //firstStrategy(taskManager,gdt);
    //second_strategy(taskManager,gdt);
    third_strategy(taskManager,gdt);
    //fourth_strategy(taskManager,gdt);
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

