#include <multiboot.h>

extern void pmm_install(struct multiboot_info *);
extern void vmm_install(void);

void mmu_install(struct multiboot_info *mbd) {
    pmm_install(mbd);
    vmm_install();
}