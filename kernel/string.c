// kernel/string.c
// Adding number conversion implementations (itoa, uitoa, reverse). Based on user string.txt.

#include "string.h"
#include <stddef.h> // For NULL
#include <stdint.h> // Need this for int types used in itoa/uitoa

// --- Basic String Functions (from user string.txt) ---

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') { // Use explicit null check
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 != '\0' && (*s1 == *s2)) { // Loop while chars match and s1 isn't null
        s1++;
        s2++;
    }
    // Return difference of characters at point of difference or end
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char *saved = dest;
    while ((*dest++ = *src++) != '\0'); // Copy includes null terminator
    return saved;
}

// Copy at most n bytes; pads with nulls if src is shorter than n.
// WARNING: May not null-terminate dest if src length >= n.
char* strncpy(char* dest, const char* src, size_t n) {
    char *saved = dest;
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for ( ; i < n; i++) { // Pad remaining with nulls
        dest[i] = '\0';
    }
    return saved;
}

// Basic strtok Implementation (from user string.txt)
// WARNING: Modifies input string! Not thread-safe!
char* strtok(char *str, const char *delim) {
    static char *saveptr = NULL;
    char *token_start;

    if (str != NULL) { // If a new string is given, reset the state
        saveptr = str;
    }
    if (saveptr == NULL || *saveptr == '\0') { // If no state or at end, return NULL
        return NULL;
    }

    // Skip leading delimiters
    while (*saveptr != '\0') {
        const char *d_ptr = delim;
        int is_delimiter = 0;
        while (*d_ptr != '\0') { if (*saveptr == *d_ptr++) { is_delimiter = 1; break; } }
        if (!is_delimiter) break; // Found start of token
        saveptr++;
    }
    if (*saveptr == '\0') { saveptr = NULL; return NULL; } // End of string reached while skipping

    token_start = saveptr; // Mark start of token

    // Find end of token
    while (*saveptr != '\0') {
        const char *d_ptr = delim;
        int is_delimiter = 0;
        while (*d_ptr != '\0') { if (*saveptr == *d_ptr++) { is_delimiter = 1; break; } }
        if (is_delimiter) break; // Found end delimiter
        saveptr++;
    }

    // Terminate token and update state for next call
    if (*saveptr != '\0') { // Found a delimiter
        *saveptr = '\0';    // Replace delimiter with null terminator
        saveptr++;          // Advance saveptr past the null terminator
    } else { // Reached end of original string
        saveptr = NULL;     // Signal end for next call
    }
    return token_start;
}


// --- NEW: Number Conversion Implementations ---

// Helper function to reverse a string in place
static void reverse(char *str, int length) {
    int start = 0;
    int end = length - 1;
    char temp;
    while (start < end) {
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// Integer to ASCII (itoa) - Signed version
char* itoa(int value, char* buffer, int base) {
    int i = 0;
    int is_negative = 0;
    unsigned int u_value; // Use unsigned to handle INT_MIN correctly

    // Validate base
    if (base < 2 || base > 36) { buffer[0] = '\0'; return buffer; }

    // Handle 0 explicitly
    if (value == 0) { buffer[i++] = '0'; buffer[i] = '\0'; return buffer; }

    // Handle negative sign only for base 10
    if (value < 0 && base == 10) {
        is_negative = 1;
        u_value = -value; // Safely convert to positive unsigned
    } else {
        u_value = (unsigned int)value; // Treat as unsigned otherwise
    }

    // Convert value to string in reverse order
    while (u_value != 0) {
        unsigned int rem = u_value % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0'; // lowercase hex letters
        u_value = u_value / base;
    }

    // Append sign if needed
    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0'; // Null terminate

    // Reverse the generated string
    reverse(buffer, i);

    return buffer;
}

// Unsigned Integer to ASCII (uitoa)
char* uitoa(unsigned int value, char* buffer, int base) {
     int i = 0;

    // Validate base
    if (base < 2 || base > 36) { buffer[0] = '\0'; return buffer; }

    // Handle 0 explicitly
    if (value == 0) { buffer[i++] = '0'; buffer[i] = '\0'; return buffer; }

    // Convert value to string in reverse order
    while (value != 0) {
        unsigned int rem = value % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0'; // lowercase hex letters
        value = value / base;
    }

    buffer[i] = '\0'; // Null terminate

    // Reverse the resulting string
    reverse(buffer, i);

    return buffer;
}