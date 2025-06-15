# Toolchain
AS = nasm
CC = gcc
LD = ld

# Automatically find sources
KERNEL_S_SOURCES = $(shell cd kernel && find -L * -type f -name '*.S')
KERNEL_C_SOURCES = $(shell cd kernel && find -L * -type f -name '*.c')

# Get object files
KERNEL_OBJS := $(addprefix bin/kernel/, $(KERNEL_S_SOURCES:.S=.S.o) $(KERNEL_C_SOURCES:.c=.c.o))

# Output image name
IMAGE_NAME = image

# Flags
ASFLAGS = -f elf32 -g -F dwarf
CCFLAGS = -m32 -std=gnu11 -g -ffreestanding -O0 -Wall -Wextra -nostdlib -I kernel/include -fno-stack-protector -Wno-unused-parameter -fno-stack-check -fno-lto -mno-sse -mno-sse2 -mno-80387 -mno-red-zone
QEMUFLAGS = -m 16M -cdrom bin/$(IMAGE_NAME).iso -rtc base=localtime -boot d -serial stdio
LDFLAGS = -m elf_i386 -Tkernel/arch/i386/linker.ld -z noexecstack

all: boot kernel iso

.PHONY: libc
libc:
	@$(MAKE) -C libc

.PHONY: apps
apps:
	@$(MAKE) -C apps

run: all
	@qemu-system-i386 $(QEMUFLAGS)

run-gdb: all
	@qemu-system-i386 $(QEMUFLAGS) -S -s

run-nographic: all
	@qemu-system-i386 $(QEMUFLAGS) -nographic

bin/kernel/%.c.o: kernel/%.c
	@echo " CC $<"
	@mkdir -p "$$(dirname $@)"
	@$(CC) $(CCFLAGS) -c $< -o $@

bin/kernel/%.S.o: kernel/%.S
	@echo " AS $<"
	@mkdir -p "$$(dirname $@)"
	@$(AS) $(ASFLAGS) -o $@ $<

kernel: $(KERNEL_OBJS)
	@echo " LD kernel/*.o"
	@$(LD) $(LDFLAGS) $^ -o bin/$(IMAGE_NAME).elf

iso:
	@echo " ISO $(IMAGE_NAME).iso"
	@grub-file --is-x86-multiboot ./bin/$(IMAGE_NAME).elf; \
	if [ $$? -eq 1 ]; then \
		echo " error: $(IMAGE_NAME).elf is not a valid multiboot file"; \
		exit 1; \
	fi
	@mkdir -p iso_root/boot/grub/
	@cp bin/$(IMAGE_NAME).elf iso_root/boot/$(IMAGE_NAME).elf
	@cp boot/grub.cfg iso_root/boot/grub/grub.cfg
	@grub-mkrescue -o bin/$(IMAGE_NAME).iso iso_root/ -quiet 2>&1 >/dev/null | grep -v libburnia | cat
	@rm -rf iso_root/

clean:
	@rm -rf bin