#pragma once
#include <stdint.h>

struct gdt_entry {
    uint16_t limit;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
} __attribute__((packed));

struct gdtr {
    uint16_t size;
    uint32_t offset;
} __attribute__((packed));

void gdt_install(void);