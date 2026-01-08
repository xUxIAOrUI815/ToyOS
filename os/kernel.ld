/* os/kernel.ld */
OUTPUT_ARCH(riscv)
ENTRY(_start)

BASE_ADDRESS = 0x80200000;

SECTIONS
{
    . = BASE_ADDRESS;

    /* 代码段 */
    /* 导出 stext (Start Text) */
    stext = .;
    .text : {
        *(.text.entry)  /* entry.S 必须放最前面 */
        *(.text .text.*)
        /* 把 trampoline (Trap入口) 对齐到页边界，方便映射 */
        . = ALIGN(4096);
        tramp_start = .;
        *(.text.trampoline)
        . = ALIGN(4096);
        tramp_end = .;
    }
    . = ALIGN(4096);
    /* 导出 etext (End Text) */
    etext = .;

    .rodata : {
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    }
    . = ALIGN(4096);
    /* 导出 erodata */
    erodata = .;

    /* 数据段 (已初始化的全局变量) */
    .data : {
        *(.data .data.*)
    }

    /* BSS段 */
    .bss : {
        . = ALIGN(8);
        sbss = .;      /* BSS 开始 */
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        . = ALIGN(8);
        ebss = .;      /* BSS 结束 */
    }

    /* 定义内核结束的地址 新增 ekernel符号*/
    . = ALIGN(4096);
    ekernel = .;
}