// kernel/io.h
// Adding number printing prototypes. Based on user ioh.txt. Readably formatted.

#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdint.h>

// --- Low-Level I/O Port Access Functions (Static Inline) --- (from user ioh.txt)
static inline void outb(uint16_t port, uint8_t val) { asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t ret; asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
static inline void outw(uint16_t port, uint16_t val) { asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port)); }
static inline uint16_t inw(uint16_t port) { uint16_t ret; asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
static inline void outl(uint16_t port, uint32_t val) { asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port)); }
static inline uint32_t inl(uint16_t port) { uint32_t ret; asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
static inline void outsw(uint16_t port, const void *addr, uint32_t count) { asm volatile ("rep outsw" : "+S"(addr), "+c"(count) : "d"(port)); }
static inline void insw(uint16_t port, void *addr, uint32_t count) { asm volatile ("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory"); }

// --- VGA Text Mode Definitions --- (from user ioh.txt)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY (uint16_t*)0xB8000
enum vga_color {
    VGA_COLOR_BLACK = 0, VGA_COLOR_BLUE = 1, VGA_COLOR_GREEN = 2, VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4, VGA_COLOR_MAGENTA = 5, VGA_COLOR_BROWN = 6, VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8, VGA_COLOR_LIGHT_BLUE = 9, VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11, VGA_COLOR_LIGHT_RED = 12, VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14, VGA_COLOR_WHITE = 15,
};

// --- VGA Function Prototypes --- (from user ioh.txt)
void term_init(void);
void term_setcolor(uint8_t fg, uint8_t bg);
void term_putentryat(char c, uint8_t color, size_t x, size_t y);
void term_putchar(char c);
void term_write(const char* data, size_t size);
void term_writestring(const char* data);
void term_clear(void);
void term_scroll(void);
void update_cursor(int row, int col);

// --- NEW: Number Printing Function Prototypes ---
void term_print_dec(int value);
void term_print_hex(unsigned int value); // Prints "0x" prefix, lowercase hex
void term_print_hex_byte(unsigned char value); // Prints "0x" prefix, 2 uppercase hex digits

#endif // IO_H