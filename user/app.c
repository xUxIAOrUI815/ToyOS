#include <stdint.h>

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

// 返回读取的字节数
int sys_read(char *buf, int len) {
    return syscall(63, 0, (uint64_t)buf, len);
}

void sys_exit(int code) { syscall(93, code, 0, 0); }
void sys_yield() { syscall(124, 0, 0, 0); }

void main() {
    sys_write("\n=== Welcome to ToyOS Shell ===\n");
    sys_write("Type something and I will echo it back.\n");
    sys_write("Press 'q' to quit.\n");
    
    char c;
    while (1) {
        // 打印提示符
        sys_write("> ");
        
        // 读取一个字符
        if (sys_read(&c, 1) > 0) {
            // 回显 (Echo)
            // 注意：有些终端回车是 \r，有些是 \n
            if (c == '\r') {
                sys_write("\n");
            } else {
                char buf[2] = {c, '\0'};
                sys_write(buf);
                sys_write("\n");
            }
            
            // 退出命令
            if (c == 'q') {
                sys_write("Bye!\n");
                break;
            }
        }
    }
    sys_exit(0);
}