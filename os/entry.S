# 内核的启动入口，负责在 CPU 刚启动，没有任何运行环境的情况下
# 初始化最基本的执行环境（主要是栈）并跳转到C语言的main函数
    .section .text.entry
    .globl _start
_start:
    # 1. 设置栈指针 sp (Stack Pointer)
    # 栈是向下生长的，所以我们要把它设在分配空间的顶部
    la sp, boot_stack_top

    # 2. 跳转到 C 语言的 main 函数
    call main

    # 3. 如果 main 返回了（不应该发生），这就死循环
    loop:
        j loop

    # --- 定义栈空间 ---
    .section .bss.stack
    .globl boot_stack_lower_bound
boot_stack_lower_bound:
    .space 4096 * 16  # 分配 64KB 的栈空间
    .globl boot_stack_top
boot_stack_top: