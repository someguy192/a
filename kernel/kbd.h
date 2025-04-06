// kernel/kbd.h
#ifndef KBD_H
#define KBD_H

void kbd_init(void);
char kbd_getchar(void); // Blocking call to get a character

#endif // KBD_H