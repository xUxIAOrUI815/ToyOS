#include <stdint.h>

void printf(char *fmt, ...);
extern void __switch(uint64_t *current_cx_ptr, uint64_t *next_cx_ptr);
extern void __restore_to_user();

// 1. 定义任务上下文 (必须只定义一次！)
typedef struct {
    uint64_t ra;
    uint64_t sp;
    uint64_t s[12];
} TaskContext;

#define MAX_APP_NUM 4
#define APP_BASE_ADDRESS 0x80400000
#define APP_SIZE_LIMIT 0x20000

// 2. 定义任务控制块
typedef struct {
    uint64_t kernel_stack[4096 / 8]; // 内核栈
    uint64_t user_stack[4096 / 8];   // 用户栈
    TaskContext context;             // 切换上下文
    int is_running;                  // 状态
} TaskControlBlock;

// 全局变量
TaskControlBlock tasks[MAX_APP_NUM];
int current_task_id = 0;
int app_num = 0;

extern uint64_t _app_start;
extern uint64_t _app_end;

// 3. 任务初始化函数
void task_init() {
    printf("[Kernel] Initializing tasks...\n");
    app_num = 3; 
    
    // 搬运代码
    uint64_t *src = &_app_start;
    uint64_t *dst = (uint64_t *)APP_BASE_ADDRESS;
    uint64_t *end = &_app_end;
    while (src < end) *dst++ = *src++;
    asm volatile("fence.i");

    for (int i = 0; i < app_num; i++) {
        uint64_t kstack_top = (uint64_t)&tasks[i].kernel_stack[4096/8];
        
        // 初始化 switch 上下文
        extern void __restore();
        tasks[i].context.ra = (uint64_t)__restore_to_user;
        tasks[i].context.sp = kstack_top;
        
        // 构造 Trap 上下文
        typedef struct {
            uint64_t x[32];
            uint64_t sstatus;
            uint64_t sepc;
        } TrapContext;
        
        kstack_top -= sizeof(TrapContext);
        TrapContext *cx = (TrapContext *)kstack_top;
        tasks[i].context.sp = kstack_top;
        
        cx->sstatus = 0; // User Mode
        cx->sepc = APP_BASE_ADDRESS;
        cx->x[2] = (uint64_t)&tasks[i].user_stack[4096/8];
        
        tasks[i].is_running = 1;
        printf("[Kernel] Task %d created.\n", i);
    }
}

// 4. 调度器
void schedule() {
    int next_id = (current_task_id + 1) % app_num;
    
    while (tasks[next_id].is_running == 0) {
        next_id = (next_id + 1) % app_num;
        if (next_id == current_task_id) {
            printf("[Kernel] All tasks finished!\n");
            while(1);
        }
    }
    
    int prev_id = current_task_id;
    current_task_id = next_id;
    
    printf("[Kernel] Switch %d -> %d\n", prev_id, next_id);
    // printf("[Kernel] Switching task... \n");
    
    // 注意：这里的强制类型转换必须有括号 (uint64_t *)
    __switch((uint64_t *)&tasks[prev_id].context, 
             (uint64_t *)&tasks[next_id].context);
}

void task_yield() {
    schedule();
}

void task_exit() {
    tasks[current_task_id].is_running = 0;
    schedule();
}