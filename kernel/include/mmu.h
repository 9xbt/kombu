#pragma once
#include <stddef.h>
#include <multiboot.h>

#define PAGE_SIZE       0x1000
#define PAGE_SIZE_HUGE  0x200000

#define KERNEL_PHYS_BASE    0x00100000
#define KERNEL_VIRT_BASE    0xC0000000

#define PTE_PRESENT 1ul
#define PTE_WRITABLE 2ul
#define PTE_USER 4ul
#define PTE_ACCESSED 32ul

#define VIRTUAL(ptr) ((void *)((uintptr_t)ptr) + 0xC0000000)
#define PHYSICAL(ptr) ((void *)((uintptr_t)ptr) - 0xC0000000)

#define DIV_CEILING(x, y) (x + (y - 1)) / y
#define ALIGN_UP(x, y) (DIV_CEILING(x, y) * y)
#define ALIGN_DOWN(x, y) ((x / y) * y)

void  mmu_install(struct multiboot_info *mbd);
void  mmu_mark_used(void *ptr, size_t page_count);
void *mmu_alloc(size_t page_count);
void  mmu_free(void *ptr, size_t page_count);
void  mmu_map_4mb(void *virt, void *phys, uint32_t flags);
void  mmu_unmap_4mb(void *virt);
void  mmu_map(void *virt, void *phys, uint32_t flags);
void  mmu_unmap(void *virt);
void  mmu_map_pages(int count, void *virt, void *phys, uint32_t flags);
void  mmu_unmap_pages(int count, void *virt);
void  mmu_destroy_user_pd(uintptr_t *pd);
uintptr_t  mmu_get_physical(uint32_t *pd, uintptr_t virt);
uintptr_t *mmu_create_user_pd(void);