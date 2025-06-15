# kombu
kombu is a microkernel operating system for Pentium class machines

## Building
To build, the following packages are required:
- dev-essential
- nasm
- grub
- xorriso
- mtools
- qemu-system-x86

After installing them, simply run `make run -j($nproc)` and kombu will launch in QEMU.
