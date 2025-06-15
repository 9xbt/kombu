#include <stdarg.h>
#include <printf.h>
#include <arch/i386/idt.h>

void __panic(char *file, int line, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[1024] = {-1};
    vsprintf(buf, fmt, args);
    va_end(args);

    printf("%s:%d: Kernel panic: %s\n", file, line, buf);

    struct stackframe *frame_ptr = __builtin_frame_address(0);

    printf("%s:%d: Traceback (most recent call first):\n", __FILE__, __LINE__);
    for (int i = 0; i < 10 && frame_ptr->ebp; i++) {
        printf("  #%d  0x%p in %s\n", i, frame_ptr->eip, "unknown");
        frame_ptr = frame_ptr->ebp;
    }

    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}