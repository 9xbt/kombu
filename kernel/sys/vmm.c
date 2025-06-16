#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <mmu.h>
#include <panic.h>
#include <printf.h>
#include <multiboot.h>

uint32_t *active_pd = NULL, *kernel_pd = NULL;

extern void vmm_load_pd(uint32_t *);
extern void vmm_enable_paging(void);

void vmm_flush_tlb(uintptr_t virt) {
    __asm__ volatile ("invlpg (%0)" ::"r"(virt) : "memory");
}

uint32_t *vmm_get_page_table(uint32_t *pd, uint32_t pd_index, uint32_t flags, bool alloc) {
    if (pd[pd_index] & PTE_PRESENT) {
        return (uint32_t *)(pd[pd_index] & ~0xFFF);
    }
    
    if (!alloc) {
        printf("%s:%d: \033[33mwarning:\033[0m couldn't get page table\n", __FILE__, __LINE__);
        return NULL;
    }

    uint32_t *new_pt = (uint32_t*)mmu_alloc(1);
    if (!new_pt) {
        panic("Failed to allocate page table");
    }
    
    memset(new_pt, 0, PAGE_SIZE);
    pd[pd_index] = (uintptr_t)new_pt | flags;
    return new_pt;
}

void mmu_map_4mb(void *virt, void *phys, uint32_t flags) {
    uint32_t pdindex = (uintptr_t)virt >> 22;
    
    /* Set the page size bit (bit 7) for 4MB pages */
    active_pd[pdindex] = ((uintptr_t)phys & ~0x3FFFFF) | flags | (1 << 7);
    vmm_flush_tlb((uintptr_t)virt);
}

void mmu_unmap_4mb(void *virt) {
    uint32_t pdindex = (uintptr_t)virt >> 22;
    
    if (active_pd[pdindex] & PTE_PRESENT) {
        active_pd[pdindex] = 0;
        vmm_flush_tlb((uintptr_t)virt);
    }
}

void mmu_map(void *virt, void *phys, uint32_t flags) {
    uint32_t pdindex = (uintptr_t)virt >> 22;
    uint32_t ptindex = ((uintptr_t)virt >> 12) & 0x03FF;

    uint32_t *pt = vmm_get_page_table(active_pd, pdindex, PTE_PRESENT | PTE_WRITABLE | PTE_USER, true);
    if (!pt) {
        panic("Failed to get page table for mapping");
    }

    pt[ptindex] = ((uintptr_t)phys & ~0xFFF) | flags; /* map the page */
    vmm_flush_tlb((uintptr_t)virt); /* flush the tlb entry */
}

void mmu_unmap(void *virt) {
    uint32_t pdindex = (uintptr_t)virt >> 22;
    uint32_t ptindex = ((uintptr_t)virt >> 12) & 0x03FF;

    uint32_t *pt = vmm_get_page_table(active_pd, pdindex, 0, false);
    if (!pt) return;

    /* clear the page table entry */
    pt[ptindex] = 0;

    /* check if the page table is empty and free it */
    bool empty = true;
    for (int i = 0; i < 1024; i++) {
        if (pt[i] & PTE_PRESENT) {
            empty = false;
            break;
        }
    }

    if (empty) {
        mmu_free(pt, 1);
        active_pd[pdindex] = 0;
    }

    vmm_flush_tlb((uintptr_t)virt);
}

void mmu_map_pages(int count, void *virt, void *phys, uint32_t flags) {
    for (int i = 0; i < count * PAGE_SIZE; i += PAGE_SIZE) {
        mmu_map(phys + i, virt + i, flags);
    }
}

void mmu_unmap_pages(int count, void *virt) {
    for (int i = 0; i < count * PAGE_SIZE; i += PAGE_SIZE) {
        mmu_unmap(virt + i);
    }
}

uintptr_t mmu_get_physical(uint32_t *pd, uintptr_t virt) {
    uint32_t pdindex = virt >> 22;
    uint32_t ptindex = (virt >> 12) & 0x03FF;

    if (!(pd[pdindex] & PTE_PRESENT)) return 0;
    
    if (pd[pdindex] & (1 << 7)) {
        return (pd[pdindex] & ~0x3FFFFF) | (virt & 0x3FFFFF);
    }
    
    uint32_t *pt = (uint32_t *)(pd[pdindex] & ~0xFFF);
    if (!(pt[ptindex] & PTE_PRESENT)) return 0;
    
    return (pt[ptindex] & ~0xFFF) | (virt & (PAGE_SIZE - 1));
}

void mmu_free_page_directory(uint32_t *pd) {
    if (!pd) return;

    for (int i = 0; i < 1024; i++) {
        if (!(pd[i] & PTE_PRESENT)) continue;
        
        /* skip 4MB pages, they don't have page tables */
        if (pd[i] & (1 << 7)) {
            pd[i] = 0;
            continue;
        }
        
        uint32_t *pt = (uint32_t *)(pd[i] & ~0xFFF);
        mmu_free(pt, 1);
        pd[i] = 0;
    }
}

uintptr_t *mmu_create_user_pd(void) {
    uint32_t *new_pd = (uint32_t *)mmu_alloc(1);
    memset(new_pd, 0, PAGE_SIZE);
    
    for (int i = 768; i < 1024; i++) {
        new_pd[i] = kernel_pd[i];
    }
    
    mmu_map_4mb(NULL, NULL, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
    mmu_map_4mb((void *)0x400000, (void *)0x400000, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
    return new_pd;
}

void mmu_destroy_user_pd(uintptr_t *pd) {
    if (!pd) return;
    
    for (int i = 0; i < 768; i++) {
        if (!(pd[i] & PTE_PRESENT)) continue;
        
        /* skip 4MB pages */
        if (pd[i] & (1 << 7)) {
            pd[i] = 0;
            continue;
        }
        
        uint32_t *pt = (uint32_t *)(pd[i] & ~0xFFF);
        mmu_free(pt, 1);
        pd[i] = 0;
    }
    
    mmu_free(pd, 1);
}

void vmm_direct_map_4mb(uint32_t *pd, uintptr_t virt, uintptr_t phys, uint32_t flags) {
    uint32_t pdindex = virt >> 22;
    pd[pdindex] = (phys & ~0x3FFFFF) | flags | (1 << 7);
}

void vmm_install(void) {
    active_pd = kernel_pd = mmu_alloc(1);
    memset(kernel_pd, 0, PAGE_SIZE);
    mmu_map_4mb(0, 0, PTE_PRESENT | PTE_WRITABLE | PTE_USER);

    printf("%s:%d: done mapping kernel regions\n", __FILE__, __LINE__);
    
    vmm_load_pd(kernel_pd);
    vmm_enable_paging();

    printf("%s:%d: successfully enabled paging\n", __FILE__, __LINE__);
}