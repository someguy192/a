// kernel/ide.h
// Basic IDE/PATA PIO Driver Definitions. Readably formatted.

#ifndef IDE_H
#define IDE_H

#include <stdint.h>
#include <stddef.h>

// --- Constants ---

// Primary IDE Bus I/O Ports (Standard PC Addresses)
#define IDE_DATA_REG        0x1F0 // Read/Write Data Register (16-bit)
#define IDE_ERROR_REG       0x1F1 // Read Error Register
#define IDE_FEATURES_REG    0x1F1 // Write Features Register
#define IDE_SECTOR_COUNT_REG 0x1F2 // Read/Write Sector Count (Number of sectors to transfer)
#define IDE_LBA_LOW_REG     0x1F3 // Read/Write LBA bits 0-7
#define IDE_LBA_MID_REG     0x1F4 // Read/Write LBA bits 8-15
#define IDE_LBA_HIGH_REG    0x1F5 // Read/Write LBA bits 16-23
#define IDE_DRIVE_HEAD_REG  0x1F6 // Read/Write Drive Select & LBA bits 24-27
#define IDE_STATUS_REG      0x1F7 // Read Status Register
#define IDE_COMMAND_REG     0x1F7 // Write Command Register

#define IDE_ALT_STATUS_REG  0x3F6 // Read Alternate Status Register (doesn't clear interrupt)
#define IDE_DEV_CTRL_REG    0x3F6 // Write Device Control Register (reset, disable IRQ)

// Status Register Bits (Read from 0x1F7 or 0x3F6)
#define IDE_STATUS_ERR      (1 << 0) // Error occurred (check Error Register)
#define IDE_STATUS_IDX      (1 << 1) // Index mark (unused)
#define IDE_STATUS_CORR     (1 << 2) // Corrected data (unused)
#define IDE_STATUS_DRQ      (1 << 3) // Data Request Ready (controller ready for data transfer)
#define IDE_STATUS_SRV      (1 << 4) // Service Request (overlapped mode)
#define IDE_STATUS_DF       (1 << 5) // Drive Fault / Write Fault
#define IDE_STATUS_RDY      (1 << 6) // Drive Ready (Spinup complete, ready for commands)
#define IDE_STATUS_BSY      (1 << 7) // Busy (Controller is processing command)

// Command Register Codes (Write to 0x1F7)
#define IDE_CMD_READ_PIO    0x20 // Read Sectors with Retry (PIO)
#define IDE_CMD_WRITE_PIO   0x30 // Write Sectors with Retry (PIO)
#define IDE_CMD_FLUSH_CACHE 0xE7 // Write Cache Flush (essential after writes)
#define IDE_CMD_IDENTIFY    0xEC // Identify Drive (get drive parameters)

// Drive/Head Register Bits (Write to 0x1F6)
// For LBA28 mode:
// Bit 7: Must be 1
// Bit 6: LBA mode select (1=LBA, 0=CHS) -> Must be 1
// Bit 5: Must be 1
// Bit 4: Drive select (0=Master, 1=Slave)
// Bits 3-0: LBA bits 24-27
#define IDE_LBA_MODE_BASE   0xE0 // Sets bits 7, 6, 5 for LBA mode (master assumed initially)


// --- Function Prototypes ---

// Initialize the IDE driver (basic checks/setup). Returns 0 on success.
int ide_initialize();

// Reads 'count' sectors starting from LBA 'lba' into 'buffer'.
// Assumes buffer is large enough (count * 512 bytes).
// Uses Primary Master drive via PIO polling. Blocking call.
// Returns 0 on success, negative error code on failure.
int read_sectors(uint32_t lba, uint16_t count, void* buffer);

// Writes 'count' sectors starting from LBA 'lba' from 'buffer'.
// Assumes buffer contains valid data (count * 512 bytes).
// Uses Primary Master drive via PIO polling. Blocking call. Includes cache flush.
// Returns 0 on success, negative error code on failure.
int write_sectors(uint32_t lba, uint16_t count, const void* buffer);

#endif // IDE_H