#pragma once
#include <stddef.h>
#include <multiboot.h>

#define PAGE_SIZE       0x1000
#define PAGE_SIZE_HUGE  0x200000

#define KERNEL_PHYS_BASE    0x00100000
#define KERNEL_VIRT_BASE    0xC0000000

#define DIV_CEILING(x, y) (x + (y - 1)) / y
#define ALIGN_UP(x, y) (DIV_CEILING(x, y) * y)
#define ALIGN_DOWN(x, y) ((x / y) * y)

void  mmu_install(struct multiboot_info *mbd);
void  mmu_mark_used(void *ptr, size_t page_count);
void *mmu_alloc(size_t page_count);
void  mmu_free(void *ptr, size_t page_count);