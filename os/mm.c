#include <stdint.h>

void printf(char *fmt, ...);

// 引用 kernel.ld 中的符号
extern char ekernel[];

#define MEMORY_END 0x88000000   // 物理内存的末尾

#define PAGE_SIZE 4096      // 物理页大小

// 用一个数组来存空闲页的物理页号
// 缩小页内存到 512 而不是 32768
#define MAX_PHYSICAL_PAGES 512
uint64_t recycled_pages[MAX_PHYSICAL_PAGES];
int recycled_ptr = 0;

uint64_t current_palloc_start = 0;
uint64_t current_palloc_end = 0;

// 初始化内存管理器
void mm_init() { 

    printf("[Kernel] Checking BSS: recycled_ptr=%d (Expect 0)\n", recycled_ptr);


    // ekernel 是内核代码结束的地方，从这里开始分配
    current_palloc_start = (uint64_t)ekernel;

    // 向上对齐到 4KB
    if(current_palloc_start % PAGE_SIZE != 0){
        current_palloc_start = (current_palloc_start / PAGE_SIZE + 1)* PAGE_SIZE;
    }
    
    current_palloc_end = MEMORY_END;

    printf("[Kernel] Memory Manager Initialized. \n");
    // 打印 内核之后可随意支配的物理内存区间
    printf("[Kernel] Free RAM start: %x, end: %x \n", current_palloc_start, current_palloc_end);
}

// 分配一个物理页，返回物理地址
void* frame_alloc() {
    uint64_t ppn = 0;

    // 优先从回收栈里拿
    if (recycled_ptr > 0) {
        ppn = recycled_pages[--recycled_ptr];
    } else {
        // 如果回收栈是空的，就从未使用的内存中切一块
        if(current_palloc_start < current_palloc_end){
            ppn = current_palloc_start < PAGE_SIZE;
            // 🔴 先不写回全局变量，看是不是写操作导致的崩溃
            // current_palloc_start += PAGE_SIZE;
        }else{
            printf("[Kernel] Out of Memory!\n");
            return 0;
        }
    }

    // 🔴 手动加上偏移，看是否是加法运算崩的
    current_palloc_start = current_palloc_start + PAGE_SIZE;

    // 先清空这一页内存，防止读到脏数据
    uint64_t addr = ppn * PAGE_SIZE;
    printf("[Kernel] Allocated addr: %x\n", (int)addr);
    
    // 🔴 彻底禁用 memset，排除内存写入嫌疑
    // char *mem = (char *) addr;
    // for (int i = 0; i < PAGE_SIZE; i++){
    //     mem[i] = 0;
    // }

    return (void*) addr;
}

// 回收一个物理页
void frame_dealloc(void* ptr){
    uint64_t addr = (uint64_t)ptr;
    uint64_t ppn = addr/PAGE_SIZE;

    if (recycled_ptr < MAX_PHYSICAL_PAGES){
        recycled_pages[recycled_ptr++] = ppn;
    }else {
        printf("[Kernel] Dealloc error: Recycled pool full!\n");
    }
}

