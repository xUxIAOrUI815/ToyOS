// os/paging.c
#include <stdint.h>

void printf(char *fmt, ...);
void* frame_alloc();

// --- 寄存器操作 ---
#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64_t)pagetable) >> 12))

// --- 页表项标志位 ---
#define PTE_V (1L << 0) // valid: 页表项有效
#define PTE_R (1L << 1) // read: 可读
#define PTE_W (1L << 2) // write: 可写
#define PTE_X (1L << 3) // execute: 可执行
#define PTE_U (1L << 4) // user: 用户态可访问
#define PTE_A (1L << 6) // accessed: 被访问过
#define PTE_D (1L << 7) // dirty: 被修改过

// 宏定义
#define PAGE_SIZE 4096
// 从 PTE 中取出 物理页号 PPN
#define PTE2PPN(pte) (((pte) >> 10) & 0x0FFFFFFFFFFFFFL)
// 将物理页号 PPN 转换为 PTE
#define PPN2PTE(ppn) (((ppn) << 10))
// 获取物理地址
#define PTE2PA(pte) (PTE2PPN(pte) * PAGE_SIZE)
// 获取虚拟地址的某一级索引
#define PX(level, va) ((((uint64_t)(va)) >> (12 + 9 * (level))) & 0x1FF)

// 页表结构 一个页表页包含 512 个 PTE
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

// --- 页表查询与创建 ---
// 查找页表 如果中间缺失页表页 则创建

uint64_t* walk(pagetable_t pagetable, uint64_t va, int alloc) {
    for (int level = 2; level > 0; level--) {
        int idx = PX(level, va);
        uint64_t pte = pagetable[idx];
        if (pte & PTE_V) {
            // 如果该项有效 说明下一级页表存在
            // 取出下一级页表的物理页号 转为物理地址
            pagetable = (pagetable_t)PTE2PA(pte);
        } else {
            // 如果无效 说明缺页
            if (!alloc) return 0;

            // 分配一个新的物理页作为下一级页表
            pagetable_t new_page = (pagetable_t)frame_alloc();
            if (new_page == 0) return 0;    // 内存不足

            // 将新页填入当前页表项
            // (uint64_t)new_page / PAGE_SIZE 即 PPN
            // | PTE_V 表示有效
            pagetable[idx] = PPN2PTE((uint64_t)new_page / PAGE_SIZE) | PTE_V;

            // 更新 pagetable 指针 
            pagetable = new_page;
        }
    }

    // 循环结束，pagetable 现在指向 Level 0 (最底层) 页表
    // 返回对应的 PTE 指针
    return &pagetable[PX(0, va)];
}

// 建立映射 将虚拟地址 va 映射到物理地址 pa 
// perm 权限标志位 R/W/X/U
int mappages(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm) {
    uint64_t start = va;
    uint64_t end = va + size;
    uint64_t offset = pa - start;

    for (uint64_t a = start; a <= end; a += PAGE_SIZE) {
        uint64_t *pte = walk(pagetable, a, 1);
        if (pte == 0) return -1;
        
        // 如果是 Map Text 阶段，打印一下当前进度
        // 这样我们知道是在第几页崩的
        if (va == (uint64_t)stext) {
             // 减少打印频率，只打印每 4KB
             printf("Mapping VA %x\n", a);
        }

        if (*pte & PTE_V) {
            // printf("Remap warning: %x\n", a);
        }
        *pte = PPN2PTE((a + offset) / PAGE_SIZE) | perm | PTE_V | PTE_A | PTE_D;
    }
    return 0;
}

// 创建用户页表
// 分配根页表
// 映射 Trap入口  所有进程都必须有，否则无法进入内核
pagetable_t uvm_create(){
    pagetable_t pagetable = (pagetable_t) frame_alloc();
    if(pagetable == 0) return 0;

    // 映射 Trampoline 到虚拟地址最高处 (与内核页表保持一致)
    // TODO: 先返回空页表
    return pagetable;
}

// 给用户页表添加映射
// va: 用户虚拟地址
// pa: 物理地址
// size: 大小
// perm: 权限 (比如 PTE_R | PTE_W | PTE_U)
void uvm_map(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm) {
    if (mappages(pagetable, va, pa, size, perm | PTE_U) != 0) {
        printf("[Kernel] uvm_map failed!\n");
        while(1);
    }
}

// 内核页表指针
pagetable_t kernel_pagetable;

// 创建内核页表
void kvminit() {
    kernel_pagetable = (pagetable_t)frame_alloc();

    // printf("[Kernel] Kernel PT created at %x\n", kernel_pagetable);
    printf("[Kernel] stext=%x, etext=%x\n", (uint64_t)stext, (uint64_t)etext);
    printf("[Kernel] Text Size=%x\n", (uint64_t)etext - (uint64_t)stext);

    // 1. 映射 UART 
    // 权限: R | W
    mappages(kernel_pagetable, UART0, UART0, PAGE_SIZE, PTE_R | PTE_W);
    printf("[Kernel] Map UART... done.\n");

    // 2. 映射内核代码段 (.text)
    // 权限: R | X
    // mappages(kernel_pagetable, (uint64_t)stext, (uint64_t)stext, 
    //          (uint64_t)etext - (uint64_t)stext, PTE_R | PTE_X);
    // printf("[Kernel] Map Text... done.\n");
    printf("[Kernel] Start mapping Text...\n");
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
    // 把它做 1:1 映射到虚拟地址最高处，和内核其他部分分开
    // mappages(kernel_pagetable, (uint64_t)tramp_start, (uint64_t)tramp_start, PAGE_SIZE, PTE_R | PTE_X);
}

// 开启分页
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


void* frame_alloc();
void uvm_map(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm);

// 简单的内存复制
void my_memcpy_paging(void *dst, void *src, uint64_t len) {
    char *d = dst; char *s = src;
    while(len--) *d++ = *s++;
}

// 从父页表复制内存给子页表
// old_pt: 父进程页表
// new_pt: 子进程页表
// start/sz: 用户空间范围 (0 ~ 0xXXXXX)
int uvm_copy(pagetable_t old_pt, pagetable_t new_pt, uint64_t sz) {
    uint64_t start = 0;
    
    // 遍历用户空间的每一页
    for (uint64_t va = start; va < sz; va += PAGE_SIZE) {
        // 1. 在父页表中找到 PTE
        uint64_t *old_pte = walk(old_pt, va, 0);
        if (!old_pte || !(*old_pte & PTE_V)) {
            continue; // 如果父进程没用这页，跳过
        }
        
        // 2. 获取父进程这页的物理地址
        uint64_t pa = PTE2PA(*old_pte);
        // 获取权限 (屏蔽掉 R/W/X 以外的位，比如 A/D)
        int flags = (*old_pte) & 0x3FF; 

        // 3. 为子进程分配一个新的物理页
        void *new_pa = frame_alloc();
        if (new_pa == 0) return -1; // 内存不足
        
        // 4. 【关键】把父进程的数据拷贝到新页
        my_memcpy_paging(new_pa, (void*)pa, PAGE_SIZE);
        
        // 5. 在子进程页表中建立映射
        // flags 包含了 PTE_U 等标志
        uvm_map(new_pt, va, (uint64_t)new_pa, PAGE_SIZE, flags);
    }
    return 0;
}