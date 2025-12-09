#include <stdint.h>

void printf(char *fmt, ...);
extern void __switch(uint64_t *current_cx_ptr, uint64_t *next_cx_ptr);

// 任务上下文 对应 switch.S 保存的内容
typedef struct {
    uint64_t ra;
    uint64_t sp;
    uint64_t s[12];
} TaskContext;

#define MAX_APP_NUM 4
#define APP_BASE_ADDRESS 0x80400000
#define APP_SIZE_LIMIT 0X20000

// 任务控制块
typedef struct {
    uint64_t kernel_stack[4096/8];      // 每个任务自己的内核栈
    uint64_t user_stack[4096/8];        // 每个任务自己的用户栈
    TaskContext context;        // 任务切换上下文
    int is_running;             // 0: 空闲/运行完 1: 运行中
} TaskContext;

TaskControlBlock tasks[MAX_APP_NUM];
int current_task_id = 0;
int app_num = 0;

// 外部引用
extern uint64_t _app_start;
extern uint64_t _app_end;

// 初始化任务
// 让所有任务都运行同一个 App 代码，但是用不同的栈
void task_init() {
    printf("[Kernel] Initializing tasks... \n");
    app_num = 3;    // 创建 3 个进程来跑这个代码

    // 把 App 代码搬运到内存 只搬运一次 大家共用一个代码段
    uint64_t *src = &_app_start;
    uint64_t *dst = (uint64_t *)APP_BASE_ADDRESS;
    uint64_t *end = &_app_end;
    while (src < end) *dst++ = *src++;
    asm volatile("fence.i");

    for (int i = 0; i < app_num; i++){
        // 1. 设置内核栈顶
        uint64_t kstack_top = (uint64_t)&tasks[i].kernel_stack[4096/8];

        // 2. 初始化 switch 上下文
        // 当 switch 切换到这里时，会跳转到 __restore (trap_entry.S)
        extern void __restore();
        tasks[i].context.ra = (uint64_t)__restore;
        tasks[i].context.sp = kstack_top;

        // 3. 构造 Trap 上下文 放在内核栈顶
        typedef struct {
            uint64_t x[32];
            uint64_t sstatus;
            uint64_t sepc;
        }TrapContext;

        // 预留空间
        kstack_top -= sizeof(TrapContext);
        TrapContext *cx = (TrapContext *)kstack_top;

        // 修正 switch 里的 sp 让它指向 TrapContext 下面
        tasks[i].context.sp = kstack_top;

        cx->sstatus = 0;    // User Mode
        cx->sepc = APP_BASE_ADDRESS;    // 入口

        // 每个任务用自己的 User Stack
        cx->x[2] = (uint64_t)&tasks[i].user_stack[4096/8];

        tasks[i].is_running = 1;
        printf("[Kernel] Task %d created.\n", i);
    }
}

// 调度器 找到下一个任务并切换
void schedule() {
    int next_id = (current_task_id + 1) % app_num;

    // 使用轮转调度
    // 当一个任务完成时，找下一个任务
    while(tasks[next_id].is_running == 0){
        next_id = (next_id + 1) % app_num;
        if(next_id == current_task_id){
            printf("[Kernel] All tasks finished! \n");
            while(1);
        }
    }
    int prev_id = current_task_id;
    current_task_id = next_id;

    printf("[Kernel] Switch %d -> %d \n", prev_id, next_id);

    __switch(uint64_t *)&tasks[prev_id].context,
            (uint64_t *)&tasks[next_id].context;
}

// 供 sys_yield 调用
void task_yield(){
    schedule();
}

// 供 sys_exit 调用
void task_exit(){
    tasks[current_task_id].is_running = 0;
    schedule();
}