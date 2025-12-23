OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    /* 应用程序被加载到的固定地址 */
    /* . = 0x80400000; */
    . = 0x10000;
    
    .text : {
        *(.text .text.*)
    }
    /* 显式包含 rodata */
    .rodata : {
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    }
    .data : {
        *(.data .data.*)
    }
    .bss : {
        *(.bss .bss.*)
    }
}