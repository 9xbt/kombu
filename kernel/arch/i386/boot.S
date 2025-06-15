[BITS 32]

section .text.prologue
    global _start
    extern kmain

_start:
    cli
    mov esp, stack_top

    push eax
    push ebx
    call kmain
    ud2

section .bss
align 16
stack_bottom:
    resb 8192
stack_top: