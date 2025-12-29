// os/sbi.c
typedef unsigned long long uint64;

// 内联汇编：调用 OpenSBI
inline uint64 sbi_call(uint64 which, uint64 arg0, uint64 arg1, uint64 arg2) {
    register uint64 a0 asm("a0") = arg0;
    // 调用 OpenSBI 服务
    // SBI 调用寄存器传递参数
    // 返回值放在 a0 中
    register uint64 a1 asm("a1") = arg1;
    register uint64 a2 asm("a2") = arg2;
    register uint64 a7 asm("a7") = which;
    
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a7)
                 : "memory");
    return a0;
}

// 输出单个字符
void console_putchar(int c) {
    // 1 代表 Legacy SBI 的 Console Putchar 扩展
    sbi_call(1, c, 0, 0);
}

// 打印字符串 (为了方便 main 使用)
void console_putstr(char *str) {
    while (*str) {
        console_putchar(*str++);
    }
}

// 从控制台读取一个字符
// 返回值：如果是 -1 表示没有输入，否则返回字符的 ASCII 码
long console_getchar(){
    return sbi_call(2,0,0,0);
}