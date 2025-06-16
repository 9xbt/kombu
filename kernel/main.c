#include <arch/i386/idt.h>
#include <arch/i386/gdt.h>
#include <mmu.h>
#include <malloc.h>
#include <printf.h>
#include <version.h>
#include <multiboot.h>

__attribute__((section(".multiboot")))
struct multiboot_header mboot_header = {
    .magic = MULTIBOOT_MAGIC,
    .flags = MULTIBOOT_FLAGS,
    .checksum = -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)
};

void kmain(struct multiboot_info *mboot_info, uint32_t mboot_magic) {
    printf("%s %d.%d.%d %s %s\n",
        __kernel_name, __kernel_version_major, __kernel_version_minor,
        __kernel_version_patch, __kernel_build_date, __kernel_build_time);
    
    gdt_install();
    idt_install();
    mmu_install(mboot_info);
    kernel_heap = heap_initialize();

    uintptr_t *fault = (uintptr_t *)0xdeadbeef;
    *fault = 123;

    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}