#include <stdint.h>

// --- 系统调用封装 ---
int syscall(int which, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    register uint64_t a0 asm("a0") = arg0;
    register uint64_t a1 asm("a1") = arg1;
    register uint64_t a2 asm("a2") = arg2;
    register uint64_t a7 asm("a7") = which;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7) : "memory");
    return a0;
}

void sys_write(char *str) {
    int len = 0;
    while(str[len]) len++;
    syscall(64, 1, (uint64_t)str, len);
}

int sys_read(char *buf, int len) {
    return syscall(63, 0, (uint64_t)buf, len);
}

int sys_fork() {
    return syscall(220, 0, 0, 0);
}

void sys_exit(int code) { syscall(93, code, 0, 0); }
void sys_yield() { syscall(124, 0, 0, 0); }

// --- 字符串辅助函数 ---
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

// --- 核心：读取一行输入 ---
void readline(char *buf, int max_len) {
    int i = 0;
    char c;
    
    while (1) {
        // 读取 1 个字符
        sys_read(&c, 1);
        
        // 1. 处理回车 (CR \r 或 LF \n)
        if (c == '\r' || c == '\n') {
            sys_write("\n"); // 换行
            buf[i] = '\0';   // 字符串结束符
            break;
        }
        
        // 2. 处理退格 (Backspace: 127 或 \b)
        else if (c == 127 || c == '\b') {
            if (i > 0) {
                // 逻辑删除
                i--;
                // 视觉删除：退格 -> 打印空格覆盖 -> 再退格
                sys_write("\b \b");
            }
        }
        
        // 3. 处理普通字符
        else {
            if (i < max_len - 1) {
                buf[i++] = c;
                // 回显字符 (Echo)
                char tmp[2] = {c, '\0'};
                sys_write(tmp);
            }
        }
    }
}

// --- Shell 主逻辑 ---
void main() {
    sys_write("I am the Parent. Calling fork()...\n");
    
    int pid = sys_fork();
    
    if (pid == 0) {
        // 子进程逻辑
        sys_write("  [Child] I am the child! I'm running!\n");
        sys_write("  [Child] I'm going to yield.\n");
        sys_yield();
        sys_write("  [Child] I'm back. Bye!\n");
        sys_exit(0);
    } else if (pid > 0) {
        // 父进程逻辑
        sys_write("  [Parent] Fork success! Child PID = ");
        
        sys_write("(child_pid)\n");
        
        sys_write("  [Parent] I will wait (yield) for child.\n");
        sys_yield();
        sys_write("  [Parent] I am back. Bye!\n");
        sys_exit(0);
    } else {
        sys_write("  [Parent] Fork failed!\n");
    }
}