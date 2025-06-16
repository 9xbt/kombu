#include <printf.h>
#include <stdint.h>
#include <arch/i386/idt.h>
#include <arch/i386/pic.h>

__attribute__((aligned(0x10)))
struct idt_entry idt_entries[256];
struct idtr idt_descriptor;
extern void *idt_int_table[];
void *irq_handlers[256];

const char *isr_errors[32] = {
    "division by zero",
    "debug",
    "non-maskable interrupt",
    "breakpoint",
    "detected overflow",
    "out-of-bounds",
    "invalid opcode",
    "no coprocessor",
    "double fault",
    "coprocessor segment overrun",
    "bad TSS",
    "segment not present",
    "stack fault",
    "general protection fault",
    "page fault",
    "unknown interrupt",
    "coprocessor fault",
    "alignment check",
    "machine check",
    "SIMD floating-point exception",
    "virtualization exception",
    "control protection exception",
    "reserved",
    "hypervisor injection exception",
    "VMM communication exception",
    "security exception",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved"
};

void idt_install(void) {
    for (uint16_t i = 0; i < 256; i++) {
        idt_set_entry(i, (uint32_t)idt_int_table[i], 0x08, 0x8E);
    }

    idt_descriptor = (struct idtr) {
        .size = sizeof(struct idt_entry) * 256 - 1,
        .offset = (uint32_t)idt_entries
    };

    asm volatile ("lidt %0" :: "m"(idt_descriptor));
    printf("%s:%d: IDT address: 0x%p\n", __FILE__, __LINE__, (uint32_t)&idt_descriptor);

    pic_install(0x20, 0x28);
    asm volatile ("sti");
}

void idt_reinstall(void) {
    asm volatile ("lidt %0" :: "m"(idt_descriptor));
    asm volatile ("sti");
}

void idt_set_entry(uint8_t index, uint32_t base, uint16_t selector, uint8_t type) {
    idt_entries[index].base_low = base & 0xFFFF;
    idt_entries[index].selector = selector;
    idt_entries[index].zero = 0x00;
    idt_entries[index].type = type;
    idt_entries[index].base_high = (base >> 16) & 0xFF;
}

void irq_register(uint8_t vector, void *handler) {
    irq_handlers[vector] = handler;
}

void irq_unregister(uint8_t vector) {
    irq_handlers[vector] = (void *)0;
}

void isr_handler(struct registers r) {
    if (r.int_no == 0xff) {
        return;
    }

    printf("%s:%d: x86 Fault: \033[91m%s\033[0m\n"
        "edi: 0x%p esi: 0x%p ebp:    0x%p\n"
        "esp: 0x%p ebx: 0x%p edx:    0x%p\n"
        "ecx: 0x%p eax: 0x%p eip:    0x%p\n"
        "cs:  0x%p ss:  0x%p eflags: 0x%p\n",
        __FILE__, __LINE__, isr_errors[r.int_no], r.edi, r.esi, r.ebp,
        r.esp, r.ebx, r.edx, r.ecx, r.eax, r.eip, r.cs, r.ss, r.eflags);
    if (r.int_no == 14) {
        uint32_t cr2;
        asm volatile("mov %%cr2, %0" : "=r" (cr2));
        printf("%s:%d: Faulting address: 0x%p\n", __FILE__, __LINE__, cr2);

        printf("%s:%d: %s %s %s\n", __FILE__, __LINE__,
            r.err_code & 0x01 ? "Page-protection violation," : "Page not present,",
            r.err_code & 0x02 ? "write operation," : "read operation,",
            r.err_code & 0x04 ? "user mode" : "kernel mode");
    }

    struct stackframe *frame_ptr = __builtin_frame_address(0);

    printf("%s:%d: Traceback (most recent call first):\n", __FILE__, __LINE__);
    for (int i = 0; i < 10 && frame_ptr->ebp; i++) {
        printf("  #%d  0x%p in %s\n", i, frame_ptr->eip, "unknown");
        frame_ptr = frame_ptr->ebp;
    }

    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}

void irq_handler(struct registers r) {
    void(*handler)(struct registers *);
    handler = irq_handlers[r.int_no - 32];

    if (handler) {
        handler(&r);
    }
}