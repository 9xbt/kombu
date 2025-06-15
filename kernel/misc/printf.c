#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <arch/i386/vga.h>

int hex_length(uint32_t val) {
    int len = 0;
    do {
        len++; val >>= 4;
    } while (val != 0);
    return len;
}

void parse_num(char *s, int *ptr, int32_t val, uint32_t base, bool is_signed) {
    if (is_signed && val < 0) {
        s[(*ptr)++] = '-';
        val = -val;
    }
    uint32_t n = (uint32_t)val / base;
    int r = (uint32_t)val % base;
    if (val >= (int64_t)base) {
        parse_num(s, ptr, n, base, is_signed);
    }
    s[(*ptr)++] = (r + '0');
}

void parse_hex(char *s, int *ptr, uint32_t val, int i) {
    if (!i) {
        i = hex_length(val);
    }
    while (i-- > 0) {
        s[(*ptr)++] = "0123456789abcdef"[val >> (i * 4) & 0x0F];
    }
}

void parse_string(char *s, int *ptr, char *str) {
    if (!str) {
        memcpy(s + *ptr, "(null)", 6);
        *ptr += 6;
        return;
    }
    strcpy(s + *ptr, str);
    *ptr += strlen(str);
}

int vsprintf(char *s, const char *fmt, va_list args) {
    int ptr = 0;

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;

            switch (*fmt) {
                case 'u':
                    parse_num(s, &ptr, va_arg(args, int), 10, false);
                    break;
                case 'd':
                    parse_num(s, &ptr, va_arg(args, int), 10, true);
                    break;
                case 'x':
                    parse_hex(s, &ptr, va_arg(args, uint32_t), 0);
                    break;
                case 'p':
                    parse_hex(s, &ptr, va_arg(args, uint32_t), 8);
                    break;
                case 's':
                    parse_string(s, &ptr, va_arg(args, char *));
                    break;
                case 'c':
                    s[ptr++] = (char)va_arg(args, int);
                    break;
            }
        } else {
            s[ptr++] = *fmt;
        }
        fmt++;
    }

    return 0;
}

int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[1024] = {0};
    int ret = vsprintf(buf, fmt, args);
    puts(buf);
    va_end(args);
    return ret;
}