#include <stdint.h>
#include <printf.h>
#include <arch/i386/gdt.h>

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

struct gdt_entry gdt_entries[5];
struct gdtr gdt_descriptor;

void gdt_flush(void) {
    asm volatile (
        "lgdt %0\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ss\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "push $0x8\n"
        "push $1f\n"
        "lret\n"
        "1:\n"
        : : "m"(gdt_descriptor) : "ax", "memory"
    );
}

void gdt_set_entry(uint8_t index, uint16_t limit, uint32_t base, uint8_t access, uint8_t gran) {
    gdt_entries[index].limit = limit;
    gdt_entries[index].base_low = base & 0xFFFF;
    gdt_entries[index].base_mid = (base >> 16) & 0xFF;
    gdt_entries[index].access = access;
    gdt_entries[index].gran = gran;
    gdt_entries[index].base_high = (base >> 24) & 0xFF;
}

void gdt_install(void) {
    gdt_set_entry(0, 0x0000, 0x00000000, 0x00, 0x00);
    gdt_set_entry(1, 0xFFFF, 0x00000000, 0x9B, 0xCF);
    gdt_set_entry(2, 0xFFFF, 0x00000000, 0x93, 0xCF);
    gdt_set_entry(3, 0xFFFF, 0x00000000, 0xFB, 0xCF);
    gdt_set_entry(4, 0xFFFF, 0x00000000, 0xF3, 0xCF);

    gdt_descriptor = (struct gdtr) {
        .size = sizeof(struct gdt_entry) * 5 - 1,
        .offset = (uint32_t)&gdt_entries
    };

    gdt_flush();
    printf("%s:%d: GDT address: 0x%p\n", __FILE__, __LINE__, (uint32_t)&gdt_descriptor);
}