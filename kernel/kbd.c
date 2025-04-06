// kernel/kbd.c
// Complete File - Basic PS/2 Keyboard Driver (Polling)

#include "kbd.h"     // Our keyboard header
#include "io.h"      // Include io.h for inb() function !!
#include <stdint.h>  // For uint8_t etc.
#include <stddef.h>  // For NULL (potentially used later)

// --- Low-level I/O function DEFINITION REMOVED ---
// static inline uint8_t inb(uint16_t port) { ... } // <<< REMOVED (Now in io.h)

// --- Keyboard Controller Ports ---
#define KBD_DATA_PORT   0x60 // Read: Scancode; Write: Send Command Data
#define KBD_STATUS_PORT 0x64 // Read: Status Register
#define KBD_CMD_PORT    0x64 // Write: Send Command to Controller

// Status Register Bits (Port 0x64 Read)
#define KBD_STATUS_OBF  0x01 // Output Buffer Full (data available from keyboard/port 0x60)
#define KBD_STATUS_IBF  0x02 // Input Buffer Full (controller busy, don't write to 0x60/0x64)
// Other bits exist but are less critical for basic polling

// --- Scancode Map (US Keyboard Layout, Scan Code Set 1) ---
// This is a basic map for key presses only. It does not handle Shift, Ctrl, Alt,
// key releases, Caps Lock, Num Lock, etc.
const char scancode_map[128] = {
      0,  0x1B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', // 0x00 - 0x0E (0=Null, 0x1B=ESC, \b=Backspace)
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // 0x0F - 0x1C (\t=Tab, \n=Enter)
      0, /*LCtrl*/ 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', // 0x1D - 0x29 (Left Ctrl)
      0, /*LShift*/'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, /*RShift*/ // 0x2A - 0x36
    '*', /*Keypad **/   0, /*LAlt*/ ' ', /*CapsLock*/                          // 0x37 - 0x3A (Space)
    // --- F-Keys, Other Keys (Mapped to 0 - Ignored) ---
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /* F1-F10 */           // 0x3B - 0x44
      0, /*NumLock*/ 0, /*ScrollLock*/                                        // 0x45 - 0x46
    // --- Keypad Numbers ---
      0, /*KP 7*/ 0, /*KP 8*/ 0, /*KP 9*/ '-', /*KP -*/                       // 0x47 - 0x4A
      0, /*KP 4*/ 0, /*KP 5*/ 0, /*KP 6*/ '+', /*KP +*/                       // 0x4B - 0x4E
      0, /*KP 1*/ 0, /*KP 2*/ 0, /*KP 3*/ 0, /*KP 0*/ '.', /*KP .*/           // 0x4F - 0x53
      0, 0, 0, 0, /* F11, F12 */ 0, 0,                                       // 0x54 - 0x58
    // --- বাকি সব 0 ---
    // Release codes (scancode | 0x80) are ignored by kbd_getchar currently.
};


// --- Public Keyboard Functions ---

// Initializes keyboard (currently does nothing for basic polling).
// Could be used later for setting scan code sets or enabling interrupts.
void kbd_init(void) {
    // Future: Can add code here to reset keyboard controller, set LEDs, etc.
}

// Checks if data is available from the keyboard.
// Returns non-zero if data ready, 0 otherwise.
int kbd_is_data_ready(void) {
    // Read status port and check the Output Buffer Full bit
    return inb(KBD_STATUS_PORT) & KBD_STATUS_OBF;
}

// Reads a raw scancode from the keyboard data port.
// NOTE: This function assumes data is ready; call kbd_is_data_ready() first,
// or use kbd_getchar() which handles polling.
uint8_t kbd_read_scancode(void) {
    // Read byte from data port
    return inb(KBD_DATA_PORT);
}

// Blocking function to get the next ASCII character from the keyboard.
// - Waits (polls) until a key is pressed.
// - Reads the scancode.
// - Ignores key release events (scancodes >= 0x80).
// - Translates known press scancodes using `scancode_map`.
// - Ignores modifier keys (Shift, Ctrl, Alt) and unmapped keys.
// - Returns the ASCII character (or special chars like '\n', '\b', '\t').
char kbd_getchar(void) {
    uint8_t scancode;
    char ascii_char;

    while (1) {
        // --- Poll Status Port ---
        // Wait until the Output Buffer Full bit (OBF) is set in the status register.
        while (!kbd_is_data_ready()) {
            // Yield CPU slightly while polling (optional but good practice)
            asm volatile("pause" ::: "memory");
        }

        // --- Read Scancode ---
        // Data is ready, read the scancode byte from the data port.
        scancode = kbd_read_scancode(); // Uses inb() via io.h

        // --- Process Scancode ---
        // Check if it's a key press event (release codes have the highest bit set)
        if (!(scancode & 0x80)) { // If highest bit (0x80) is NOT set... it's a press event
            // Translate scancode to ASCII using the map, if valid index
            if (scancode < sizeof(scancode_map)) {
                ascii_char = scancode_map[scancode];

                // Check if the scancode mapped to a valid character (not 0)
                if (ascii_char != 0) {
                    // Valid character found, return it.
                    return ascii_char;
                }
                // else: Scancode mapped to 0 (e.g., Shift key, F-key), ignore and continue loop.
            }
            // else: Scancode is outside our map's range, ignore and continue loop.
        }
        // else: Scancode had highest bit set (>= 0x80), meaning it's a key release event.
        // Ignore release events in this simple driver and continue loop.

    } // End while(1) loop - continues until a valid character is returned
}