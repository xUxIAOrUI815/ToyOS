.section .text
.globl __switch
.align 2

# __switch(current_task_cx_ptr, next_task_cx_ptr)
# a0: 当前任务上下文指针的地址  保存到哪个位置
# a1: 下一个任务上下文指针的地址  从哪恢复
__switch:
    # 1. 保存当前任务的 Callee-Saved 寄存器
    # sp 是内核栈指针
    sd sp, 8(a0)
    sd ra, 0(a0)
    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    # 2. 恢复下一个任务的寄存器
    ld ra, 0(a1)
    ld sp, 8(a1)
    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)

    # 当 ret 执行时, CPU 会跳转到下一个任务上次停下的地方
    ret         
