#include <multiboot.h>

extern void pmm_install(struct multiboot_info *);

void mmu_install(struct multiboot_info *mbd) {
    pmm_install(mbd);
}