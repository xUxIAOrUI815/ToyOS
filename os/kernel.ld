/* os/kernel.ld */
OUTPUT_ARCH(riscv)
ENTRY(_start)

BASE_ADDRESS = 0x80200000;

SECTIONS
{
    . = BASE_ADDRESS;

    /* 代码段 */
    .text : {
        *(.text.entry)  /* entry.S 必须放最前面 */
        *(.text .text.*)
    }

    /* 只读数据段 (如字符串 "Hello") */
    .rodata : {
        *(.rodata .rodata.*)
    }

    /* 数据段 (已初始化的全局变量) */
    .data : {
        *(.data .data.*)
    }

    /* BSS段 */
    /* .bss : {
        *(.bss .bss.*)
        *(.sbss .sbss.*)
    }*/
    .bss : {
        . = ALIGN(8);
        sbss = .;      /* BSS 开始 */
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        . = ALIGN(8);
        ebss = .;      /* BSS 结束 */
    }

    /* 定义内核结束的地址 */
    . = ALIGN(4096);
    ekernel = .;
}