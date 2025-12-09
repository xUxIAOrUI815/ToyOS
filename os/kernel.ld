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

    /* BSS段 (未初始化的变量，自动清零) */
    .bss : {
        *(.bss .bss.*)
        *(.sbss .sbss.*)
    }
}