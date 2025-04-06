// kernel/string.h
// Adding number conversion prototypes.

#ifndef STRING_H
#define STRING_H

#include <stddef.h> // For size_t

// --- Basic String Functions (from your string.txt) ---
size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strtok(char *str, const char *delim);

// --- NEW: Number to String Conversion Prototypes ---
char* itoa(int value, char* buffer, int base); // Signed version
char* uitoa(unsigned int value, char* buffer, int base); // Unsigned version

#endif // STRING_H