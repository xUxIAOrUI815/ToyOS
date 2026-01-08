#include <stdint.h>

// 引用外部函数
void printf(char *fmt, ...);
void console_putchar(int c);
void task_exit();
void task_yield();
long console_getchar();
int task_fork();        // 注册 fork 的系统调用

typedef struct {
    uint64_t x[32];
    uint64_t sstatus;
    uint64_t sepc;
} TrapContext;

// 让 syscall 也返回 TrapContext*，保持数据流连贯
TrapContext* syscall(TrapContext *cx) {
    uint64_t syscall_num = cx->x[17];

    if (syscall_num == 64) { // sys_write
        uint64_t fd = cx->x[10];
        char *buf = (char *)cx->x[11];
        uint64_t len = cx->x[12];
        
        // 调试打印 (确认指针正常)
        // printf("[Kernel] sys_write: fd=%d, buf=%x, len=%d\n", fd, buf, len);
        
        for(int i=0; i<len; i++) {
            console_putchar(buf[i]);
        }
        
        cx->x[10] = len;
        cx->sepc += 4;
    } 
    else if (syscall_num == 93) { // sys_exit
        // printf("[Kernel] Application exited with code %d\n", cx->x[10]);
        // // 这里的死循环是为了防止跑飞
        // while(1);
        // 切换任务
        printf("[Kernel] App %d exited. \n", cx->x[10]);

        task_exit();
    } 
    else if(syscall_num == 124){
        // 124 常用的 yield 调用号
        task_yield();
        cx->sepc += 4;  // 返回后继续执行下一条指令
    }
    else if(syscall_num == 63){
        uint64_t fd = cx->x[10];
        char *buf = (char *)cx->x[11];
        uint64_t len = cx->x[12];

        // 只支持标准输入(fd=0)
        if(fd == 0){
            long c;
            while(1){
                c = console_getchar();
                if(c != -1) break;
            }
            *buf = (char) c;
            cx->x[10] = 1;
        }else{
            cx->x[10] = 0;
        }
        cx->sepc += 4;
    }
    else if (syscall_num == 220) {  // sys_fork
        cx->x[10] = task_fork();
        cx->sepc += 4;
    }
    else {
        printf("[Kernel] Unknown syscall: %d\n", syscall_num);
        while(1);
    }
    
    // 必须返回 cx
    return cx;
}


TrapContext* trap_handler(TrapContext *cx) {
    uint64_t scause, stval;
    asm volatile("csrr %0, scause" : "=r"(scause));
    asm volatile("csrr %0, stval" : "=r"(stval));
    
    // 判断是不是中断
    if ((scause >> 63) == 1) {
        // ... (暂不处理)
    } else {
        if (scause == 8) {
            cx = syscall(cx);
        } else {
            // 打印详细崩溃信息
            printf("\n[Kernel] PANIC! Exception @ Kernel Mode\n");
            printf("scause = %d (Exception Type)\n", scause);
            printf("stval  = %x (Bad Address)\n", stval);
            printf("sepc   = %x (Instruction Address)\n", cx->sepc);
            while(1);
        }
    }
    return cx;
}
