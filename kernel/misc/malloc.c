#include <mmu.h>
#include <printf.h>
#include <malloc.h>

struct heap kernel_heap;

void *kmalloc(size_t n) {
    return heap_alloc(&kernel_heap, n);
}

void kfree(void *ptr) {
    heap_free(ptr);
}

struct heap heap_initialize(void) {
    struct heap h;
    h.head = (struct heap_block *)VIRTUAL(mmu_alloc(1));
    mmu_map(h.head, PHYSICAL(h.head), PTE_PRESENT | PTE_WRITABLE);
    h.head->next = h.head;
    h.head->prev = h.head;
    h.head->size = 0;

    printf("%s:%d: heap head address: 0x%p\n", __FILE__, __LINE__, h.head);
    return h;
}

void *heap_alloc(struct heap *h, size_t n) {
    if (n == 0) {
        printf("%s:%d: \033[33mwarning:\033[0m allocating 0 bytes\n", __FILE__, __LINE__);
    }

    size_t pages = DIV_CEILING(sizeof(struct heap_block) + n, PAGE_SIZE);
    
    struct heap_block *block = (struct heap_block *)VIRTUAL(mmu_alloc(pages));
    if (!block) {
        printf("%s:%d: allocation failed\n", __FILE__, __LINE__);
        return NULL;
    }
    mmu_map_pages(pages, block, PHYSICAL(block), PTE_PRESENT | PTE_WRITABLE);
    block->next = h->head;
    block->prev = h->head->prev;
    block->size = n;

    return (void *)block + sizeof(struct heap_block);
}

void heap_free(void *ptr) {
    struct heap_block *block = (struct heap_block *)(ptr - sizeof(struct heap_block));

    /* TODO: implement & check checksum */

    block->prev->next = block->next;
    block->next->prev = block->prev;
    size_t pages = DIV_CEILING(sizeof(struct heap_block) + block->size, PAGE_SIZE);

    mmu_free(PHYSICAL(block), pages);
    mmu_unmap_pages(pages, block);
}