#pragma once
#include <stddef.h>

struct heap_block {
    struct heap_block *next;
    struct heap_block *prev;
    size_t size;
};

struct heap {
    struct heap_block *head;
};

extern struct heap kernel_heap;

void *kmalloc(size_t n);
void  kfree(void *ptr);

struct heap heap_initialize();
void *heap_alloc(struct heap *h, size_t n);
void  heap_free(void *ptr);