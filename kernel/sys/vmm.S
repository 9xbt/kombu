section .text

extern active_pd
global vmm_load_pd
vmm_load_pd:
    push ebp
    mov ebp, esp
    mov eax, [esp + 8]
    mov [active_pd], eax
    mov cr3, eax
    mov esp, ebp
    pop ebp
    ret

global vmm_enable_paging
vmm_enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr4
    or eax, 0x10        ; PSE bit
    mov cr4, eax
    mov eax, cr0
    or eax, 0x80000000  ; PG bit
    mov cr0, eax
    mov esp, ebp
    pop ebp
    ret