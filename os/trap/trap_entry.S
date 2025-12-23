# os/trap/trap_entry.S
# 🔴【修改】把 .section .text 改为 .section .text.trampoline
.section .text.trampoline
.globl __alltraps
.globl __restore
.align 2

__alltraps:
    # 从用户态陷入内核时保存上下文
    csrrw sp, sscratch, sp
    # 分配 34 * 8 = 272 字节的栈空间
    # 32 个通用寄存器 和 2 个CSR(sstatus 和 sepc)
    addi sp, sp, -34*8
    
    # 保存通用寄存器
    sd x1, 1*8(sp)
    # x2 (sp) saved later
    sd x3, 3*8(sp)
    sd x4, 4*8(sp)
    sd x5, 5*8(sp)
    sd x6, 6*8(sp)
    sd x7, 7*8(sp)
    sd x8, 8*8(sp)
    sd x9, 9*8(sp)
    sd x10, 10*8(sp)
    sd x11, 11*8(sp)
    sd x12, 12*8(sp)
    sd x13, 13*8(sp)
    sd x14, 14*8(sp)
    sd x15, 15*8(sp)
    sd x16, 16*8(sp)
    sd x17, 17*8(sp)
    sd x18, 18*8(sp)
    sd x19, 19*8(sp)
    sd x20, 20*8(sp)
    sd x21, 21*8(sp)
    sd x22, 22*8(sp)
    sd x23, 23*8(sp)
    sd x24, 24*8(sp)
    sd x25, 25*8(sp)
    sd x26, 26*8(sp)
    sd x27, 27*8(sp)
    sd x28, 28*8(sp)
    sd x29, 29*8(sp)
    sd x30, 30*8(sp)
    sd x31, 31*8(sp)
    
    # 记录 trap 前的特权级、中断使能
    csrr t0, sstatus
    csrr t1, sepc
    sd t0, 32*8(sp)     
    sd t1, 33*8(sp)     
    
    csrr t2, sscratch   
    sd t2, 2*8(sp)      
    
    mv a0, sp          
    call trap_handler

__restore:
    # 从内核恢复到用户态时恢复上下文
    mv sp, a0
    .globl __restore_to_user
    
__restore_to_user:
    # 从内核恢复到用户态时恢复上下文
    ld t0, 32*8(sp)
    ld t1, 33*8(sp)
    csrw sstatus, t0
    csrw sepc, t1
    
    ld x1, 1*8(sp)
    ld x3, 3*8(sp)
    ld x4, 4*8(sp)
    ld x5, 5*8(sp)
    ld x6, 6*8(sp)
    ld x7, 7*8(sp)
    ld x8, 8*8(sp)
    ld x9, 9*8(sp)
    ld x10, 10*8(sp)
    ld x11, 11*8(sp)
    ld x12, 12*8(sp)
    ld x13, 13*8(sp)
    ld x14, 14*8(sp)
    ld x15, 15*8(sp)
    ld x16, 16*8(sp)
    ld x17, 17*8(sp)
    ld x18, 18*8(sp)
    ld x19, 19*8(sp)
    ld x20, 20*8(sp)
    ld x21, 21*8(sp)
    ld x22, 22*8(sp)
    ld x23, 23*8(sp)
    ld x24, 24*8(sp)
    ld x25, 25*8(sp)
    ld x26, 26*8(sp)
    ld x27, 27*8(sp)
    ld x28, 28*8(sp)
    ld x29, 29*8(sp)
    ld x30, 30*8(sp)
    ld x31, 31*8(sp)
    
    ld x2, 2*8(sp)
    csrw sscratch, x2
    addi sp, sp, 34*8
    csrrw sp, sscratch, sp
    
    sret