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

// 计算字符串长度后，调用系统调用64，向控制台打印字符串
void sys_write(char *str) {
    int len = 0;
    while(str[len]) len++;
    syscall(64, 1, (uint64_t)str, len);
}

// 调用63，从控制台读取指定长度的字符
int sys_read(char *buf, int len) {
    return syscall(63, 0, (uint64_t)buf, len);
}

// 调用93，终止当前进程并返回退出码
void sys_exit(int code) { syscall(93, code, 0, 0); }

// 调用124，主动让出 CPU
void sys_yield() { syscall(124, 0, 0, 0); }

// 调用220，创建子进程
int sys_fork() { return syscall(220, 0, 0, 0); }

// --- 字符串比较函数 ---
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

// --- 行读取器 ---
// 逐字符读取用户输入
// 处理回车 / 换行 结束输入，添加字符串结束符\0
// 处理退格键 ASCII 127 或\b，删除最后一个字符并回显删除效果
// 输入长度限制 max_len-1，预留结束符位置
// 实时回显用户输入的字符 读取到用户输入的有效字符后 立即调用 sys_write 把这个字符输出到控制台
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

// --- 模拟子进程任务 ---
// 打印子进程启动提示
// 循环5次 每次打印一个. 然后调用 sys_yield 主动让出 CPU
// 任务完成后打印提示 调用 sys_exit(0) 退出子进程
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
// 无限循环处理用户命令
// 打印提示符 ToyOS > 
// 调用 readline 读取用户输入的命令
// help 打印支持的命令列表
// test 创建子进程 父子进程并发执行
// exit 打印停机提示 调用 sys_exit(0) 退出 shell
// 空输入 无响应
// 未知命令 打印 Unknown command
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
                // 子进程逻辑
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