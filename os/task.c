// os/task.c
#include <stdint.h>

void printf(char *fmt, ...);
void* frame_alloc(); // mm.c
typedef uint64_t* pagetable_t; // paging.c
pagetable_t uvm_create();
void uvm_map(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm);
extern void __switch(uint64_t *current_cx_ptr, uint64_t *next_cx_ptr);

// --- 宏定义 ---
#define PAGE_SIZE 4096
#define MAX_APP_NUM 4

// 虚拟地址布局：
// 0x10000 -> App 代码
// 0x20000 -> App 栈 (栈底)
#define USER_CODE_START 0x10000
#define USER_STACK_START 0x20000 

// PTE 标志位
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)

typedef struct {
    uint64_t ra;
    uint64_t sp;
    uint64_t s[12];
} TaskContext;

// 调整结构体顺序防止踩踏
typedef struct {
    int is_running;
    TaskContext context;
    uint64_t kernel_stack[PAGE_SIZE / 8];
    pagetable_t pagetable;
    uint64_t trap_cx_ppn;
} TaskControlBlock;

TaskControlBlock tasks[MAX_APP_NUM];
int app_num = 0;
TaskContext idle_cx;
int current_task_id = -1;

extern uint64_t _app_start;
extern uint64_t _app_end;
extern void __restore_to_user();
extern pagetable_t kernel_pagetable;

void my_memcpy(void *dst, void *src, uint64_t len) {
    char *d = dst; char *s = src;
    while(len--) *d++ = *s++;
}

void task_init() {
    printf("[Kernel] Initializing tasks with Virtual Memory...\n");
    app_num = 1;        // 修改创建的任务数量

    uint64_t app_size = (uint64_t)&_app_end - (uint64_t)&_app_start;

    for (int i = 0; i < app_num; i++) {
        // 1. 创建用户页表 (复制内核页表)
        tasks[i].pagetable = (pagetable_t)frame_alloc();
        my_memcpy(tasks[i].pagetable, kernel_pagetable, PAGE_SIZE);

        // 2. 映射用户代码 (Text)
        void *app_mem = frame_alloc();
        my_memcpy(app_mem, &_app_start, app_size);
        
        // 🔴【关键】刷新指令缓存！防止CPU读到旧数据
        asm volatile("fence.i");

        // 映射到 0x10000, 权限 R|W|X|U
        uvm_map(tasks[i].pagetable, USER_CODE_START, (uint64_t)app_mem, PAGE_SIZE, 
                PTE_R | PTE_W | PTE_X | PTE_U);

        // 3. 映射用户栈 (Stack) - 🔴【修复点】
        void *stack_mem = frame_alloc();
        // 映射到 0x20000, 权限 R|W|U (用户可读写)
        uvm_map(tasks[i].pagetable, USER_STACK_START, (uint64_t)stack_mem, PAGE_SIZE, 
                PTE_R | PTE_W | PTE_U);

        // 4. 初始化内核栈逻辑
        uint64_t kstack_top = (uint64_t)&tasks[i].kernel_stack[PAGE_SIZE/8];
        
        tasks[i].context.ra = (uint64_t)__restore_to_user;
        tasks[i].context.sp = kstack_top;

        typedef struct {
            uint64_t x[32];
            uint64_t sstatus;
            uint64_t sepc;
        } TrapContext;
        
        kstack_top -= sizeof(TrapContext);
        TrapContext *cx = (TrapContext *)kstack_top;
        tasks[i].context.sp = kstack_top;

        cx->sstatus = (1L << 18); // SUM=1
        cx->sepc = USER_CODE_START; // 0x10000
        
        // 🔴【关键】设置用户栈指针
        // 栈向下生长，所以 SP 设为 (Start + Size)
        cx->x[2] = USER_STACK_START + PAGE_SIZE; 

        tasks[i].is_running = 1;
        printf("[Kernel] Task %d created. PT=%x\n", i, tasks[i].pagetable);
    }
}

void schedule() {
    int next_id;
    if (current_task_id == -1) next_id = 0;
    else {
        next_id = (current_task_id + 1) % app_num;
        while (tasks[next_id].is_running == 0) {
            next_id = (next_id + 1) % app_num;
            if (next_id == current_task_id) {
                printf("[Kernel] All tasks finished!\n");
                while(1);
            }
        }
    }
    
    int prev_id = current_task_id;
    current_task_id = next_id;
    
    uint64_t next_satp = (8L << 60) | (((uint64_t)tasks[next_id].pagetable) >> 12);
    asm volatile("csrw satp, %0" : : "r"(next_satp));
    asm volatile("sfence.vma");
    
    if (prev_id != -1) {
        __switch((uint64_t *)&tasks[prev_id].context, (uint64_t *)&tasks[next_id].context);
    } else {
        printf("[Kernel] Idle -> Task %d\n", next_id);
        __switch((uint64_t *)&idle_cx, (uint64_t *)&tasks[next_id].context);
    }
}

void task_yield() { schedule(); }
void task_exit() { tasks[current_task_id].is_running = 0; schedule(); }