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

void sys_exit(int code) { syscall(93, code, 0, 0); }
void sys_yield() { syscall(124, 0, 0, 0); }
int sys_fork() { return syscall(220, 0, 0, 0); }

// --- 字符串工具 ---
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

// --- 行读取器 ---
void readline(char *buf, int max_len) {
    int i = 0;
    char c;
    while (1) {
        if (sys_read(&c, 1) > 0) {
            if (c == '\r' || c == '\n') {
                sys_write("\n");
                buf[i] = '\0';
                break;
            } else if (c == 127 || c == '\b') {
                if (i > 0) {
                    i--;
                    sys_write("\b \b");
                }
            } else {
                if (i < max_len - 1) {
                    buf[i++] = c;
                    char tmp[2] = {c, '\0'};
                    sys_write(tmp);
                }
            }
        }
    }
}

// --- 模拟复杂的子进程任务 ---
void run_child_task() {
    sys_write("  [Child] I am a new process created by Shell!\n");
    sys_write("  [Child] working: ");
    for (int i = 0; i < 5; i++) {
        sys_write(".");
        // 假装在忙，让出CPU
        sys_yield();
    }
    sys_write(" Done!\n");
    sys_exit(0);
}

// --- 主程序 ---
void main() {
    char cmd[128];

    sys_write("\n");
    sys_write("++++++++++++++++++++++++++++++++++++\n");
    sys_write("   ToyOS Multitasking Shell v1.0    \n");
    sys_write("++++++++++++++++++++++++++++++++++++\n");

    while (1) {
        sys_write("ToyOS > ");
        readline(cmd, 128);

        if (strcmp(cmd, "help") == 0) {
            sys_write("Commands:\n");
            sys_write("  help - Show this message\n");
            sys_write("  test - Fork a child process to do work\n");
            sys_write("  exit - Shutdown OS\n");
        } 
        else if (strcmp(cmd, "test") == 0) {
            sys_write("[Shell] Forking new process...\n");
            int pid = sys_fork();
            
            if (pid == 0) {
                // 子进程逻辑 (不变)
                run_child_task(); 
            } else {
                sys_write("[Shell] Child created. I will toggle with child:\n");
                
                for (int i = 0; i < 5; i++) {
                    sys_write("P"); // Parent
                    sys_yield();
                }
                sys_write("\n[Shell] Parent loop done.\n");
            }
        }
        else if (strcmp(cmd, "exit") == 0) {
            sys_write("System Halt.\n");
            sys_exit(0);
        }
        else if (cmd[0] != '\0') {
            sys_write("Unknown command.\n");
        }
    }
}