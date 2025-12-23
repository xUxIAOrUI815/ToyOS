#include <stdint.h>

void printf(char *fmt, ...);
void* frame_alloc();

// 表项标志位
#define PTE_V (1L << 0) // Valid: 页表项有效
#define PTE_R (1L << 1) // Read: 可读
#define PTE_W (1L << 2) // Write: 可写
#define PTE_X (1L << 3) // Execute: 可执行
#define PTE_U (1L << 4) // User: 用户态可访问
#define PTE_G (1L << 5) // Global: 全局有效
#define PTE_A (1L << 6) // Accessed: 被访问过
#define PTE_D (1L << 7) // Dirty: 被修改过

// 辅助宏
#define PAGE_SIZE 4096

// 从 PTE 中取出物理页号 PPN
#define PTE2PPN(pte) (((pte) >> 10) & & 0x0FFFFFFFFFFFFFL)

// 将物理页号 PPN 转换成 PTE
#define PPN2PTE(ppn) (((ppn) << 10))

// 获取物理地址
#define PTE2PA(pte) (PTE2PPN(pte) * PAGE_SIZE)

// 获取虚拟地址的某一级的索引 (VPN0, VPN1, VPN2)
#define PX(level, va) ((((uint64_t)(va)) >> (12 + 9 * (level))) & 0x1FF)

// 页表结构 一个页表页包含 512 个PTE
typedef uint64_t* pagetable_t;

// 查找页表，如果中间缺失页表页，则创建
// pagetable: 根页表地址
// va: 虚拟地址
// alloc: 如果找不到，是否分配新页？
// 返回: 对应的 PTE 指针
uint64_t* walk(pagetable_t pagetable, uint64_t va, int alloc){
    // 从最高级页表开始向下查
    for (int level = 2; level > 0; level--) {
        int idx = PX(level, va);
        uint64_t pte = pagetable[idx];
        
        if (pte & PTE_V) {
            // 如果该项有效，说明下一级页表存在
            // 取出下一级页表的物理页号，转为物理地址
            pagetable = (pagetable_t)PTE2PA(pte);
        } else {
            // 如果无效（缺页）
            if (!alloc) return 0;
            
            // 分配一个新的物理页作为下一级页表
            pagetable_t new_page = (pagetable_t)frame_alloc();
            if (new_page == 0) return 0; // 内存不足
            
            // 将新页填入当前页表项
            // 这里的 (uint64_t)new_page / PAGE_SIZE 就是 PPN
            // | PTE_V 表示有效
            pagetable[idx] = PPN2PTE((uint64_t)new_page / PAGE_SIZE) | PTE_V;
            
            // 更新 pagetable 指针，继续循环
            pagetable = new_page;
        }
    }

    // 循环结束，pagetable 现在指向 Level 0 (最底层) 页表
    // 返回对应的 PTE 指针
    return &pagetable[PX(0, va)];
}

// 建立映射：将虚拟地址 va 映射到物理地址 pa
// perm: 权限标志 (R/W/X/U)
int mappages(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm) {
    uint64_t start = va;
    uint64_t end = va + size;
    
    // 向下取整到页边界
    va &= ~(PAGE_SIZE - 1); // 0xff...f000
    
    for (;;) {
        // 查找 PTE
        uint64_t *pte = walk(pagetable, va, 1);
        if (pte == 0) return -1; // 内存分配失败
        
        if (*pte & PTE_V) {
            printf("[Kernel] Remap panic: %x already mapped!\n", va);
            return -1;
        }
        
        // 填写 PTE
        // PPN | perm | PTE_V
        *pte = PPN2PTE(pa / PAGE_SIZE) | perm | PTE_V;
        
        if (va == end - PAGE_SIZE) break; // 映射完成
        
        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
    return 0;
}