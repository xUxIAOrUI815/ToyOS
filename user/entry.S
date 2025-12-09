.section .text
.globl _start
_start:
    call main
    # 如果 main 返回了，就死循环
    loop: j loop