#include <stdint.h>

void printf(char *fmt, ...);
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

void main(){
    // printf("\n[ToyOS] Phase 3: Privilege Switching\n");
    // load_and_run_app();
    // while(1){};

    printf("\n[ToyOS] Phase 4: Multiprogramming\n");

    // 初始化
    asm volatile("csrw stvec, %0" : : "r"(__alltraps));

    // 初始化任务队列
    task_init();

    // 调度
    printf("[Kernel] Starting schedule...\n");

    while(1){};
}