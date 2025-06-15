#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <mmu.h>
#include <panic.h>
#include <bitmap.h>
#include <printf.h>
#include <multiboot.h>

uint8_t *mmu_bitmap;
uint64_t mmu_bitmap_size;
uint64_t mmu_page_count;
uint64_t mmu_usable_mem;
uint64_t mmu_used_pages;

void pmm_install(struct multiboot_info *mbd) {
    extern void *end;
    uintptr_t highest_address = 0;

    /* check bit 6 to see if we have a valid memory map */
    if(!(mbd->flags >> 6 & 0x1)) {
        panic("Invalid multiboot memory map");
    }

    struct multiboot_memory_map *mmmt = NULL;
    for(uint32_t i = 0; i < mbd->mmap_length;
        i += sizeof(struct multiboot_memory_map))
    {
        mmmt = (struct multiboot_memory_map *) (mbd->mmap_addr + i);
        
        if (mmmt->addr_low < KERNEL_PHYS_BASE) {
            mmmt->type = MULTIBOOT_MEMORY_RESERVED;
            continue;
        }

        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (mmmt->addr_low >= KERNEL_PHYS_BASE && mmmt->addr_low < (uintptr_t)&end) {
                mmmt->len_low -= (uintptr_t)&end - mmmt->addr_low;
                mmmt->addr_low = (uintptr_t)&end;
            }
            
            /* Assume memory map is not in order */
            uintptr_t region_end = mmmt->addr_low + mmmt->len_low;
            if (region_end > highest_address) {
                highest_address = region_end;
            }
        }
    }

    mmu_bitmap = (uint8_t *)&end;
    mmu_page_count = highest_address / PAGE_SIZE;
    mmu_bitmap_size = ALIGN_UP(mmu_page_count / 8, PAGE_SIZE);
    memset(mmu_bitmap, 0xFF, mmu_bitmap_size);

    mmu_usable_mem = 0;
    for(uint32_t i = 0; i < mbd->mmap_length;
        i += sizeof(struct multiboot_memory_map))
    {
        mmmt = (struct multiboot_memory_map *) (mbd->mmap_addr + i);

        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
            // Mark pages as available in bitmap
            for (uint64_t j = 0; j < mmmt->len_low; j += PAGE_SIZE) {
                bitmap_clear(mmu_bitmap, (mmmt->addr_low + j) / PAGE_SIZE);
            }
            mmu_usable_mem += mmmt->len_low;
        }
    }

    mmu_mark_used(mmu_bitmap, mmu_bitmap_size / PAGE_SIZE);

    printf("%s:%d: initialized bitmap at 0x%p\n", __FILE__, __LINE__, mmu_bitmap);
    printf("%s:%d: usable memory: %uK\n", __FILE__, __LINE__, mmu_usable_mem / 1024 - mmu_used_pages * 4);
}

void mmu_mark_used(void *ptr, size_t page_count) {
    for (size_t i = 0; i < page_count * PAGE_SIZE; i += PAGE_SIZE) {
        bitmap_set(mmu_bitmap, ((uintptr_t)ptr + i) / PAGE_SIZE);
    }
    mmu_used_pages += page_count;
}

uint32_t mmu_find_pages(uint32_t page_count) {
    uint32_t pages = 0;
    uint32_t first_page = 0;

    for (uint32_t i = 0; i < mmu_page_count; i++) {
        if (!bitmap_get(mmu_bitmap, i)) {
            if (pages == 0) {
                first_page = i;
            }
            pages++;
            if (pages == page_count) {
                for (uint32_t j = 0; j < page_count; j++) {
                    bitmap_set(mmu_bitmap, first_page + j);
                }

                mmu_used_pages += page_count;
                return first_page;
            }
        } else {
            pages = 0;
        }
    }
    return 0;
}

void *mmu_alloc(size_t page_count) {
    uint32_t pages = mmu_find_pages(page_count);
    if (!pages) {
        panic("allocation failed: out of memory");
    }
    return (void *)(pages * PAGE_SIZE);
}

void mmu_free(void *ptr, size_t page_count) {
    uint32_t page = (uint32_t)ptr / PAGE_SIZE;

    if ((uintptr_t)ptr < KERNEL_PHYS_BASE || page > mmu_bitmap_size * 8) {
        printf("%s:%d: invalid deallocation @ 0x%p\n", __FILE__, __LINE__, ptr);
        return;
    }

    for (uint32_t i = 0; i < page_count; i++) {
        if (!bitmap_get(mmu_bitmap, page + i)) {
            printf("%s:%d: double free @ 0x%p\n", __FILE__, __LINE__, ptr);
            return;
        }
        bitmap_clear(mmu_bitmap, page + i);
    }
    
    mmu_used_pages -= page_count;
}