#include <stdint.h>

void printf(char *fmt, ...);
void task_init();
void schedule();
void mm_init();
void* frame_alloc();
void frame_dealloc(void *ptr);
void kvminit();
void kvminithart();
extern uint64_t _app_start;
extern uint64_t _app_end;
extern void __alltraps();
extern void __restore(uint64_t *cx);


#define APP_BASE_ADDRESS 0x80400000
#define APP_SIZE_LIMIT 0x20000  // 假设 App 不超过 128KB

// 分配两个栈
// Kernel Stack 各种中断处理用
// User Stack 用户程序用
uint8_t kernel_stack[4096 * 2];
uint8_t user_stack[4096 * 2];

typedef struct {
    uint64_t x[32];
    uint64_t sstatus;
    uint64_t sepc;
} TrapContext;


void load_and_run_app(){
    // 1. 加载 App 代码
    uint64_t *src = &_app_start;
    uint64_t *dst = (uint64_t *)APP_BASE_ADDRESS;
    uint64_t *end = &_app_end;
    while (src < end) *dst++ = *src++;

    asm volatile("fence.i");
    
    printf("[Kernel] App loaded. Preparing to switch to User Mode... \n");

    // 2. 初始化 Trap 向量表
    // 告诉 CPU 遇到 ecall 去哪里
    asm volatile("csrw stvec, %0" : : "r"(__alltraps));

    // 3. 构造一个 "伪造" 的 Trap 上下文
    // 上下文放在 Kernel Stack 的栈顶
    TrapContext *cx = (TrapContext *) (kernel_stack + sizeof(kernel_stack) - sizeof(TrapContext));

    // 设置 sstatus: 将 SPP 位设为0 User Mode
    // 1 << 5 是 SPIE 开启中断 这里简单设为 0
    cx->sstatus = 0;

    // 设置 sepc: 返回后跳转到 App 的入口地址
    cx->sepc = APP_BASE_ADDRESS;

    // 设置 sp: 用户栈的栈顶
    cx->x[2] = (uint64_t)(user_stack + sizeof(user_stack));

    // 设置 sscratch: 指向内核栈顶 给trap.S交换用
    asm volatile("csrw sscratch, %0" : : "r"(kernel_stack + sizeof(kernel_stack)));

    printf("[Kernel] Jumping to User App via __restore!\n");

    __restore((uint64_t *)cx);
}

// paging.c 的函数
typedef uint64_t* pagetable_t;
void* frame_alloc();
int mappages(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm);


void main(){
    // printf("\n[ToyOS] Phase 3: Privilege Switching\n");
    // load_and_run_app();
    // while(1){};

    // // printf("\n[ToyOS] Phase 4: Multiprogramming\n");

    // // 初始化
    // asm volatile("csrw stvec, %0" : : "r"(__alltraps));

    // // 初始化任务队列
    // task_init();

    // // 调度
    // printf("[Kernel] Starting schedule...\n");
    // schedule();
    
    // while(1){};

    // phase 4
    // printf("\n[ToyOS] Phase 5: Memory Management\n");
    // asm volatile("csrw stvec, %0" : : "r"(__alltraps));

    // // 初始化物理内存
    // mm_init();

    // // 测试内存分配
    // void *page1 = frame_alloc();
    // void *page2 = frame_alloc();
    // printf("[Kernel] Test Alloc: p1=%x, p2=%x\n", (int)(uint64_t)page1, (int)(uint64_t)page2);
    
    // // 回收page1，此时page1的地址在回收栈的栈顶，所以分配的page3应该和之前的page1一样
    // frame_dealloc(page1);
    // void *page3 = frame_alloc();
    // printf("[Kernel] Test Dealloc: p3=%x (Should be same as p1)\n", page3);

    // while(1){};

    // printf("\n[ToyOS] Phase 6: Page Table Mapping\n");
    
    // mm_init(); // 1. 初始化物理内存
    
    // // 2. 创建一个根页表
    // pagetable_t root_pt = (pagetable_t)frame_alloc();
    // printf("[Main] Root PageTable at %x\n", (int)(uint64_t)root_pt);
    
    // // 3. 测试映射
    // // 目标：将虚拟地址 0x10000 映射到物理地址 0x80200000 (内核起点)
    // // 权限：R | W | X (7 << 1)
    // // 标志：PTE_V (1)
    // int perm = (1 << 1) | (1 << 2) | (1 << 3); // R W X
    
    // printf("[Main] Mapping 0x10000 -> 0x80200000 ...\n");
    // mappages(root_pt, 0x10000, 0x80200000, 4096, perm);
    
    // // 4. 验证：手动查表 (模拟硬件行为)
    // // 0x10000 对应的 VPN2=0, VPN1=0, VPN0=16 (0x10)
    // // 我们直接看 walk 函数能不能帮我们找到
    // extern uint64_t* walk(pagetable_t pagetable, uint64_t va, int alloc);
    // uint64_t *pte = walk(root_pt, 0x10000, 0);
    
    // if (pte && (*pte & 1)) {
    //     printf("[Main] Success! PTE at %x value: %x\n", (int)(uint64_t)pte, (int)*pte);
    //     // 验证 PPN 是否正确
    //     // 0x80200 = 0x80200000 >> 12
    //     // PTE >> 10 应该是 0x200800 (因为 RISC-V PPN 在高位)
    //     // 简单验证：只要不是 0 就行
    // } else {
    //     printf("[Main] Failed! Mapping not found.\n");
    // }

    // while (1) {};

    printf("\n[ToyOS] Phase 6: Page Table Mapping\n");
    
    mm_init(); // 1. 物理内存初始化
    
    // 2. 建立内核页表
    kvminit();
    
    // 3. 开启 MMU
    kvminithart();
    
    // 4. 如果能打印这句话，说明我们活下来了！
    printf("[Kernel] System matches Physical Memory 1:1.\n");
    
    // 5. 恢复之前的任务调度 (Phase 4 的内容)
    // task_init();
    // schedule();

    while (1) {};
}