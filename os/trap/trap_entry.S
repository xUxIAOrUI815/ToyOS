# os/trap/trap_entry.S
.altmacro
.macro SAVE_GP n
    sd x\n, \n*8(sp)
.endm
.macro LOAD_GP n
    ld x\n, \n*8(sp)
.endm

.section .text.trampoline
.globl __alltraps
.globl __restore
.align 2

__alltraps:
    # 检测是否来自内核态
    # 如果 sstatus.SPP (第8位) 是 1，说明之前是内核态，不需要交换 sp
    csrr t0, sstatus
    andi t0, t0, 1 << 8
    bnez t0, trap_from_kernel
    
    # 来自用户态：交换 sp 和 sscratch
    csrrw sp, sscratch, sp

trap_from_kernel:
    addi sp, sp, -34*8
    
    sd x1, 1*8(sp)
    sd x3, 3*8(sp)
    .set n, 5
    .rept 27
        SAVE_GP %n
        .set n, n+1
    .endr
    
    csrr t0, sstatus
    csrr t1, sepc
    sd t0, 32*8(sp)
    sd t1, 33*8(sp)
    
    csrr t2, sscratch
    sd t2, 2*8(sp)
    
    mv a0, sp
    call trap_handler

__restore:
    # a0 是 TrapContext 指针
    mv sp, a0 
    
    .globl __restore_to_user
__restore_to_user:
    
    ld t0, 32*8(sp)
    ld t1, 33*8(sp)
    csrw sstatus, t0
    csrw sepc, t1
    
    ld x1, 1*8(sp)
    ld x3, 3*8(sp)
    .set n, 5
    .rept 27
        LOAD_GP %n
        .set n, n+1
    .endr
    
    ld t0, 2*8(sp)
    csrw sscratch, t0
    
    addi sp, sp, 34*8
    
    # 如果要去用户态，需要交换 sp
    csrr t0, sstatus
    andi t0, t0, 1 << 8
    bnez t0, return_to_kernel
    
    csrrw sp, sscratch, sp

return_to_kernel:
    sret