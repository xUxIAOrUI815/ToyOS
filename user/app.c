#include <stdint.h>

// 封装 syscall 
int syscall(int which, uint64_t arg0, uint64_t arg1, uint64_t arg2){
    register uint64_t a0 asm("a0") = arg0;
    register uint64_t a1 asm("a1") = arg1;
    register uint64_t a2 asm("a2") = arg2;
    register uint64_t a7 asm("a7") = which;

    asm volatile("ecall"
                : "+r"(a0)
                : "r"(a1), "r"(a2), "r"(a7)
                : "memory");

    return a0;
}

// 实现 write 函数
void sys_write(char *str){
    int len = 0;
    while(str[len]) len++;  // 计算长度
    // 64 是 sys_write 的调用号
    syscall(64, 1, (uint64_t)str, len);
}

// 实现 exit 函数
void sys_exit(int code){
    // 93 是 sys_exit 的调用号
    syscall(93, code, 0, 0);
}

void main(){
    // 通过 ecall 请求内核打印
    sys_write("Hello! I am running in User Mode (U-Mode)! \n");
    sys_write("I can do System Calls now! \n");

    sys_exit(0);
}

