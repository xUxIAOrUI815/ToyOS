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
// 用户程序在虚拟内存中的入口地址 (让 App 以为自己从 0 开始，或者 0x1000)
#define USER_BASE_ADDR 0x10000 

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

typedef struct {
    int is_running;
    TaskContext context;
    uint64_t user_stack[PAGE_SIZE / 8];   // 暂时还是放在内核里管理
    uint64_t kernel_stack[PAGE_SIZE / 8];
    pagetable_t pagetable;                // 🔴 每个任务独立的页表
    uint64_t trap_cx_ppn;                 // Trap上下文所在的物理页号
} TaskControlBlock;

TaskControlBlock tasks[MAX_APP_NUM];
int app_num = 0;
TaskContext idle_cx;
int current_task_id = -1;

extern uint64_t _app_start;
extern uint64_t _app_end;
extern void __restore_to_user();
extern pagetable_t kernel_pagetable; // paging.c

// 简单的 memcpy
void my_memcpy(void *dst, void *src, uint64_t len) {
    char *d = dst; char *s = src;
    while(len--) *d++ = *s++;
}

void task_init() {
    printf("[Kernel] Initializing tasks with Virtual Memory...\n");
    app_num = 3; 

    // App 的大小
    uint64_t app_size = (uint64_t)&_app_end - (uint64_t)&_app_start;

    for (int i = 0; i < app_num; i++) {
        // 1. 创建用户页表
        // 注意：这里我们偷个懒，直接复制内核页表作为基础
        // 这样用户态陷入内核时，内核代码依然可见
        // 在严肃的 OS 中，应该只映射 Trampoline，这里为了教学简化
        tasks[i].pagetable = (pagetable_t)frame_alloc();
        my_memcpy(tasks[i].pagetable, kernel_pagetable, PAGE_SIZE);

        // 2. 分配物理内存来存放 User App 代码
        void *app_mem = frame_alloc(); // 分配一页 (假设 App < 4KB)
        my_memcpy(app_mem, &_app_start, app_size); // 复制 User App 代码进去
        
        // 3. 建立映射：虚拟地址 0x10000 -> 刚分配的物理地址
        // 权限：R | W | X | U (用户可读写执行)
        uvm_map(tasks[i].pagetable, USER_BASE_ADDR, (uint64_t)app_mem, PAGE_SIZE, 
                PTE_R | PTE_W | PTE_X | PTE_U);

        // 4. 初始化 Trap 上下文
        // 放在内核栈顶
        uint64_t kstack_top = (uint64_t)&tasks[i].kernel_stack[PAGE_SIZE/8];
        
        // 伪造 __switch 返回地址
        tasks[i].context.ra = (uint64_t)__restore_to_user;
        tasks[i].context.sp = kstack_top;

        // 填充 TrapContext
        typedef struct {
            uint64_t x[32];
            uint64_t sstatus;
            uint64_t sepc;
        } TrapContext;
        
        kstack_top -= sizeof(TrapContext);
        TrapContext *cx = (TrapContext *)kstack_top;
        tasks[i].context.sp = kstack_top;

        // 🔴 User Status 设置
        // SPP=0 (用户态), SPIE=1 (开启中断)
        // SUM=1 (允许内核访问用户页，偷懒做法)
        cx->sstatus = (1L << 18); 
        
        // 🔴 入口地址
        // App 以为自己从 0x10000 开始跑
        cx->sepc = USER_BASE_ADDR;
        
        // 用户栈 (暂时不做映射，直接用内核里的物理地址，因为我们偷懒复制了内核页表)
        // 在完整的 uCore 中，这里应该分配新页并映射到用户高地址
        cx->x[2] = (uint64_t)&tasks[i].user_stack[PAGE_SIZE/8];

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
    
    // 🔴 切换页表
    // 计算 satp 值 (Mode=8, PPN=tasks[next].pagetable)
    uint64_t next_satp = (8L << 60) | (((uint64_t)tasks[next_id].pagetable) >> 12);
    
    // 必须在切换任务前/后切换 satp
    // 这里我们简单粗暴地在 C 语言里切 (实际上应该在 switch.S 里切更安全)
    asm volatile("csrw satp, %0" : : "r"(next_satp));
    asm volatile("sfence.vma"); // 刷新 TLB
    
    if (prev_id != -1) {
        // printf("[Kernel] Switch %d -> %d\n", prev_id, next_id);
        __switch((uint64_t *)&tasks[prev_id].context, (uint64_t *)&tasks[next_id].context);
    } else {
        printf("[Kernel] Idle -> Task %d\n", next_id);
        __switch((uint64_t *)&idle_cx, (uint64_t *)&tasks[next_id].context);
    }
}

void task_yield() { schedule(); }
void task_exit() { tasks[current_task_id].is_running = 0; schedule(); }