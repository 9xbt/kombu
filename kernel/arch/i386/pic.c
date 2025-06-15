#include <printf.h>
#include <arch/i386/io.h>
#include <arch/i386/pic.h>

void pic_install(uint8_t pic1_offset, uint8_t pic2_offset) {
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  /* starts the initialization sequence (in cascade mode) */
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	outb(PIC1_DATA, pic1_offset);               /* ICW2: Master PIC vector offset */
	outb(PIC2_DATA, pic2_offset);               /* ICW2: Slave PIC vector offset */
	outb(PIC1_DATA, 4);                         /* ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100) */
	outb(PIC2_DATA, 2);                         /* ICW3: tell Slave PIC its cascade identity (0000 0010) */
	
	outb(PIC1_DATA, ICW4_8086);                 /* ICW4: have the PICs use 8086 mode (and not 8080 mode) */
	outb(PIC2_DATA, ICW4_8086);

	/* Unmask both PICs */
	outb(PIC1_DATA, 0);
	outb(PIC2_DATA, 0);

    printf("%s:%d: remapped dual interrupt controllers\n", __FILE__, __LINE__);
}

void pic_eoi(uint8_t irq) {
	if(irq >= 8)
		outb(PIC2_COMMAND, PIC_EOI);
	outb(PIC1_COMMAND, PIC_EOI);
}