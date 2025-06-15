#pragma once
#include <stdint.h>

#define CURSOR_SIZE 2

extern uint8_t vga_x;
extern uint8_t vga_y;

void puts(const char *str);
void putchar(const char c);

void vga_clear(void);
void vga_scroll(void);
void vga_enable_cursor(void);
void vga_disable_cursor(void);
void vga_update_cursor(void);