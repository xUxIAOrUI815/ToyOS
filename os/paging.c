// os/paging.c
#include <stdint.h>

void printf(char *fmt, ...);
void* frame_alloc();

// --- 寄存器操作 ---
#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64_t)pagetable) >> 12))

// --- 标志位 ---
#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)

#define PAGE_SIZE 4096
#define PTE2PPN(pte) (((pte) >> 10) & 0x0FFFFFFFFFFFFFL)
#define PPN2PTE(ppn) (((ppn) << 10))
#define PTE2PA(pte) (PTE2PPN(pte) * PAGE_SIZE)
#define PX(level, va) ((((uint64_t)(va)) >> (12 + 9 * (level))) & 0x1FF)

typedef uint64_t* pagetable_t;

// 引用外部符号
extern char stext[];    // 代码段开始
extern char etext[];    // 代码段结束
extern char erodata[];  // 只读数据结束
extern char ekernel[];  // 内核结束
extern char tramp_start[]; // Trap 代码开始

// QEMU 的 UART 物理地址
#define UART0 0x10000000L
#define MEMORY_END 0x88000000L

// --- 核心函数 (保留之前的 walk 和 mappages) ---

uint64_t* walk(pagetable_t pagetable, uint64_t va, int alloc) {
    for (int level = 2; level > 0; level--) {
        int idx = PX(level, va);
        uint64_t pte = pagetable[idx];
        if (pte & PTE_V) {
            pagetable = (pagetable_t)PTE2PA(pte);
        } else {
            if (!alloc) return 0;
            pagetable_t new_page = (pagetable_t)frame_alloc();
            if (new_page == 0) return 0;
            pagetable[idx] = PPN2PTE((uint64_t)new_page / PAGE_SIZE) | PTE_V;
            pagetable = new_page;
        }
    }
    return &pagetable[PX(0, va)];
}

int mappages(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm) {
    uint64_t start = va;
    uint64_t end = va + size;
    va &= ~(PAGE_SIZE - 1);
    for (;;) {
        uint64_t *pte = walk(pagetable, va, 1);
        if (pte == 0) return -1;
        if (*pte & PTE_V) {
            // printf("Remap panic: %x\n", va); // 调试时不报错，方便重入
        }
        *pte = PPN2PTE(pa / PAGE_SIZE) | perm | PTE_V;
        if (va == end - PAGE_SIZE) break;
        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
    return 0;
}

// 🔴【新增】内核页表指针
pagetable_t kernel_pagetable;

// 🔴【新增】创建内核页表
void kvminit() {
    kernel_pagetable = (pagetable_t)frame_alloc();
    printf("[Kernel] Kernel PT created at %x\n", kernel_pagetable);

    // 1. 映射 UART (如果不映射，printf 会死)
    // 权限: R | W
    mappages(kernel_pagetable, UART0, UART0, PAGE_SIZE, PTE_R | PTE_W);
    printf("[Kernel] Map UART... done.\n");

    // 2. 映射内核代码段 (.text)
    // 权限: R | X
    mappages(kernel_pagetable, (uint64_t)stext, (uint64_t)stext, 
             (uint64_t)etext - (uint64_t)stext, PTE_R | PTE_X);
    printf("[Kernel] Map Text... done.\n");

    // 3. 映射只读数据段 (.rodata)
    // 权限: R
    mappages(kernel_pagetable, (uint64_t)etext, (uint64_t)etext, 
             (uint64_t)erodata - (uint64_t)etext, PTE_R);
    printf("[Kernel] Map Rodata... done.\n");

    // 4. 映射数据段 + BSS + 剩余物理内存 (.data ~ MEMORY_END)
    // 权限: R | W
    mappages(kernel_pagetable, (uint64_t)erodata, (uint64_t)erodata, 
             (uint64_t)MEMORY_END - (uint64_t)erodata, PTE_R | PTE_W);
    printf("[Kernel] Map Data/BSS/Heap... done.\n");
    
    // 5. 映射 Trampoline (Trap 入口)
    // 把它映射到虚拟地址最高处 (uCore 惯例)，也为了和内核其他部分分开
    // 暂时我们也做 1:1 映射，为了简单
    // mappages(kernel_pagetable, (uint64_t)tramp_start, (uint64_t)tramp_start, PAGE_SIZE, PTE_R | PTE_X);
}

// 🔴【新增】开启分页！
void kvminithart() {
    // 写入 satp 寄存器
    // Mode = 8 (SV39), PPN = kernel_pagetable
    uint64_t satp_val = MAKE_SATP(kernel_pagetable);
    
    // 写入寄存器
    asm volatile("csrw satp, %0" : : "r" (satp_val));
    
    // 刷新 TLB (快表)
    asm volatile("sfence.vma zero, zero");
    
    printf("[Kernel] Paging ENABLED! Hello from Virtual World!\n");
}