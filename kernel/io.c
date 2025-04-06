// kernel/io.c
// Adding number printing implementations. Based on user ioh.txt needs. Readably formatted.

#include "io.h"
#include "string.h" // For strlen AND NOW itoa/uitoa
#include <stddef.h>
#include <stdint.h>

// --- VGA Globals ---
size_t term_row; size_t term_column; uint8_t term_color; uint16_t* term_buffer;

// --- VGA Helpers ---
static inline uint16_t vga_entry(unsigned char uc, uint8_t color){return (uint16_t)uc|(uint16_t)color<<8;}
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg){return fg|(bg<<4);}
void update_cursor(int row,int col){if(row<0)row=0;if(row>=VGA_HEIGHT)row=VGA_HEIGHT-1;if(col<0)col=0;if(col>=VGA_WIDTH)col=VGA_WIDTH-1;unsigned short pos=(row*VGA_WIDTH)+col;outb(0x3D4,0x0E);outb(0x3D5,(unsigned char)((pos>>8)&0xFF));outb(0x3D4,0x0F);outb(0x3D5,(unsigned char)(pos&0xFF));}

// --- VGA Public Functions ---
void term_init(void){term_row=0;term_column=0;term_color=vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK);term_buffer=VGA_MEMORY;term_clear();}
void term_clear(void){for(size_t y=0;y<VGA_HEIGHT;y++)for(size_t x=0;x<VGA_WIDTH;x++)term_buffer[y*VGA_WIDTH+x]=vga_entry(' ',term_color);term_row=0;term_column=0;update_cursor(term_row,term_column);}
void term_setcolor(uint8_t fg, uint8_t bg){term_color=vga_entry_color((enum vga_color)fg,(enum vga_color)bg);}
void term_putentryat(char c, uint8_t color, size_t x, size_t y){if(y>=VGA_HEIGHT||x>=VGA_WIDTH)return;term_buffer[y*VGA_WIDTH+x]=vga_entry(c,color);}
void term_scroll(void){for(size_t y=0;y<VGA_HEIGHT-1;y++)for(size_t x=0;x<VGA_WIDTH;x++)term_buffer[y*VGA_WIDTH+x]=term_buffer[(y+1)*VGA_WIDTH+x];const size_t last= (VGA_HEIGHT-1)*VGA_WIDTH;for(size_t x=0;x<VGA_WIDTH;x++)term_buffer[last+x]=vga_entry(' ',term_color);term_row=VGA_HEIGHT-1;}
void term_putchar(char c){unsigned char uc=(unsigned char)c;switch(uc){case '\n':term_column=0;term_row++;break;case '\r':term_column=0;break;case '\b':if(term_column>0){term_column--;term_putentryat(' ',term_color,term_column,term_row);}else if(term_row>0){term_row--;term_column=VGA_WIDTH-1;term_putentryat(' ',term_color,term_column,term_row);}break;default:term_putentryat(uc,term_color,term_column,term_row);term_column++;break;}if(term_column>=VGA_WIDTH){term_column=0;term_row++;}if(term_row>=VGA_HEIGHT){term_scroll();}update_cursor(term_row,term_column);}
void term_write(const char*d, size_t s){for(size_t i=0;i<s;i++)term_putchar(d[i]);}
void term_writestring(const char*d){term_write(d,strlen(d));}

// --- NEW: Number Printing Function Implementations ---
void term_print_dec(int value) {
    char buffer[12]; // Max 11 digits for 32-bit signed int + sign + null
    itoa(value, buffer, 10); // Use base 10 itoa from string.c
    term_writestring(buffer);
}

void term_print_hex(unsigned int value) {
    char buffer[9]; // Max 8 hex digits for 32-bit uint + null
    uitoa(value, buffer, 16); // Use base 16 uitoa from string.c
    term_writestring("0x");
    // Optional: Add leading zeros if needed for fixed width like 0x0000ABCD
    // size_t len = strlen(buffer);
    // for (size_t i = 0; i < 8 - len; i++) term_putchar('0');
    term_writestring(buffer); // Print lowercase hex digits from itoa/uitoa
}

void term_print_hex_byte(unsigned char value) {
     term_writestring("0x");
     // Extract high and low nibbles
     char high_nibble = (value >> 4) & 0x0F;
     char low_nibble  = value & 0x0F;
     // Convert nibbles to uppercase hex characters for clarity
     term_putchar((high_nibble > 9) ? (high_nibble - 10) + 'A' : high_nibble + '0');
     term_putchar((low_nibble > 9) ? (low_nibble - 10) + 'A' : low_nibble + '0');
}