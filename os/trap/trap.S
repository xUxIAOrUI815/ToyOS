.altmacro
.macro SAVE_GP n
    sd x\n, \n*8(sp)
.endm
.macro LOAD_GP n
    ld x\n, \n*8(sp)

.section .text
.globl __alltraps
.globl __restore
.align 2

# 1. 陷入内核
__alltraps:
    # 交换 sp 和 sscratch
    # sp 指向 Kernel Stack, sscratch 指向 User Stack
    csrrw sp, sscratch, sp
    
    # 在内核栈上 分配空间 保存 32 个通用寄存器 + 2 个特殊寄存器
    addi sp, sp, -34*8

    # 保存 x1, x3-x31, x2 是sp, 后面单独处理
    sd x1, 1*8(sp)
    sd x3, 3*8(sp)
    .set n, 5
    .rept 26
        SAVE_GP %n
        .set n, n+1
    .endr

    # 保存 sstatus 和 sepc  
    # 记录 User 运行到哪里
    cssr t0, sstatus
    cssr t1, space
    sd t0, 32*8(sp)
    sd t1, 33*8(sp)

    # 保存 User 的 sp (x2)
    cssr t2, sscratch
    sd t2, 2*8(sp)

    # 调用 C 的 Trap 处理函数
    call trap_handler

# 2. 恢复现场
__restore:
    # a0 寄存器现在存放的是 Trap上下文的指针 由 main.c 传入
    # 把 sp 切换到这个上下文的位置
    mv sp, a0

    # 恢复 sstatus 和 sepc
    ld t0, 32*8(sp)
    ld t1, 32*8(sp)
    csrw sstatus, t0
    csrw sepc, t1

    # 恢复通用寄存器
    ld x1, 1*8(sp)
    ld x3, 3*8(sp)
    .set n, 5
    .rept 27
        LOAD_GP %n
        .set n, n+1
    .endr

    # 恢复 User 的 sp
    ld x2, 2*8(sp)

    # 回收栈空间
    addi sp, sp, 34*8

    # 再次交换 sp 和 sscratch (把 sp 变回 User Stack)
    csrrw sp, sscratch, sp

    # 特权级返回、
    sret
