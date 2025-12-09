OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    /* 应用程序被加载到的固定地址 */
    . = 0x80400000;
    .text : {
        *(.text .text.*)
    }
    .data : {
        *(.data .data.*)
    }
    .bss : {
        *(.bss .bss.*)
    }
}