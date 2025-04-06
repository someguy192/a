// kernel/ide.c
// Complete File - Basic IDE/PATA PIO Driver Implementation. Readably formatted.

#include "ide.h"
#include "io.h"     // For inb, outb, insw, outsw, term_*
#include <stdint.h>

// --- Helper Functions ---

// Reads the status register with appropriate delay
static inline uint8_t ide_read_status() {
    // Reading alternate status multiple times provides a needed delay
    // (approx 400ns) without affecting interrupts.
    inb(IDE_ALT_STATUS_REG);
    inb(IDE_ALT_STATUS_REG);
    inb(IDE_ALT_STATUS_REG);
    inb(IDE_ALT_STATUS_REG);
    return inb(IDE_STATUS_REG); // Read actual status
}

// Polls the IDE status register until BSY (Busy) bit clears.
// Returns status byte on success, or -1 on timeout.
static int ide_poll_busy_clear() {
    for(int i = 0; i < 100000; ++i) { // Basic timeout loop
        uint8_t status = ide_read_status();
        if (!(status & IDE_STATUS_BSY)) {
            return status; // Return status byte when not busy
        }
        // Could add asm volatile("pause"); here
    }
    term_writestring("Error: IDE BSY timeout!\n");
    return -1; // Timeout error
}

// Polls the IDE status register after a command for data transfer readiness (DRQ).
// Waits for BSY clear, then checks ERR/DF, then waits for DRQ.
// Returns 0 if DRQ is set, negative on error/timeout.
static int ide_poll_data_request() {
    int status = ide_poll_busy_clear();
    if (status < 0) {
        // BSY timeout already reported
        return -1;
    }

    // Check for errors *after* BSY clears
    if (status & IDE_STATUS_ERR) {
        term_writestring("Error: IDE ERR set after command!\n");
        // Optionally read IDE_ERROR_REG here for details
        return -2;
    }
    if (status & IDE_STATUS_DF) {
         term_writestring("Error: IDE DF (Drive Fault) set after command!\n");
        return -3;
    }
    // Could check IDE_STATUS_RDY here too if needed

    // Now wait specifically for DRQ to be set
    for(int i = 0; i < 100000; ++i) {
        status = ide_read_status();
        // Check for errors again while waiting
        if (status & IDE_STATUS_ERR) {
            term_writestring("Error: IDE ERR set while waiting for DRQ!\n");
            return -2;
        }
        if (status & IDE_STATUS_DF) {
            term_writestring("Error: IDE DF set while waiting for DRQ!\n");
            return -3;
        }
        // Check if ready for data transfer
        if (status & IDE_STATUS_DRQ) {
            return 0; // Success! Ready to read/write via data port.
        }
        // asm volatile("pause"); // Optional yield hint
    }

    term_writestring("Error: IDE DRQ timeout!\n");
    return -5; // Timeout waiting for data request
}


// --- Public Driver Functions ---

// Initialize the IDE driver. Basic version does little.
int ide_initialize() {
    // A real driver would:
    // - Disable IDE interrupts using IDE_DEV_CTRL_REG if using polling.
    // - Send IDENTIFY command to both master/slave on primary/secondary buses.
    // - Parse IDENTIFY data to detect drive type, capabilities (LBA support, DMA modes etc).
    // - Store drive information.
    // For now, just assume primary master exists and supports LBA28 PIO.
    term_writestring("IDE: Basic PIO driver initialized (polling, primary master assumed).\n");
    return 0; // Assume success
}

// Reads 'count' sectors using PIO mode.
int read_sectors(uint32_t lba, uint16_t count, void* buffer) {
    if (count == 0) return 0;

    uint8_t* current_buf_ptr = (uint8_t*)buffer;

    for (uint16_t i = 0; i < count; ++i) {
        uint32_t current_lba = lba + i;
        int poll_result;

        // --- Send READ command ---
        poll_result = ide_poll_busy_clear(); // Wait until drive is not busy
        if (poll_result < 0) return -1;

        // Setup LBA28 parameters
        outb(IDE_DRIVE_HEAD_REG, IDE_LBA_MODE_BASE | ((current_lba >> 24) & 0x0F)); // Select Master, LBA mode, LBA bits 24-27
        outb(IDE_SECTOR_COUNT_REG, 1); // We process one sector at a time
        outb(IDE_LBA_LOW_REG, (uint8_t)(current_lba & 0xFF));           // LBA bits 0-7
        outb(IDE_LBA_MID_REG, (uint8_t)((current_lba >> 8) & 0xFF));    // LBA bits 8-15
        outb(IDE_LBA_HIGH_REG, (uint8_t)((current_lba >> 16) & 0xFF)); // LBA bits 16-23

        // Issue the read command
        outb(IDE_COMMAND_REG, IDE_CMD_READ_PIO);

        // --- Transfer Data ---
        poll_result = ide_poll_data_request(); // Wait for drive to be ready to send data (DRQ)
        if (poll_result != 0) {
            term_writestring("IDE Read: Error polling for DRQ.\n");
            return poll_result;
        }

        // Read 512 bytes (256 words) from the data port
        insw(IDE_DATA_REG, current_buf_ptr, 256);

        // Move to next buffer position
        current_buf_ptr += 512;
    }
    return 0; // Success
}

// Writes 'count' sectors using PIO mode.
int write_sectors(uint32_t lba, uint16_t count, const void* buffer) {
     if (count == 0) return 0;

    const uint8_t* current_buf_ptr = (const uint8_t*)buffer;

    for (uint16_t i = 0; i < count; ++i) {
        uint32_t current_lba = lba + i;
        int poll_result;

        // --- Send WRITE command ---
        poll_result = ide_poll_busy_clear(); // Wait until not busy
        if (poll_result < 0) return -1;

        // Setup LBA28 parameters (same as read)
        outb(IDE_DRIVE_HEAD_REG, IDE_LBA_MODE_BASE | ((current_lba >> 24) & 0x0F));
        outb(IDE_SECTOR_COUNT_REG, 1);
        outb(IDE_LBA_LOW_REG, (uint8_t)(current_lba & 0xFF));
        outb(IDE_LBA_MID_REG, (uint8_t)((current_lba >> 8) & 0xFF));
        outb(IDE_LBA_HIGH_REG, (uint8_t)((current_lba >> 16) & 0xFF));

        // Issue write command
        outb(IDE_COMMAND_REG, IDE_CMD_WRITE_PIO);

        // --- Transfer Data ---
        poll_result = ide_poll_data_request(); // Wait for drive ready to RECEIVE data (DRQ)
        if (poll_result != 0) {
             term_writestring("IDE Write: Error polling for DRQ.\n");
            return poll_result;
        }

        // Write 512 bytes (256 words) to the data port
        outsw(IDE_DATA_REG, current_buf_ptr, 256);

        // --- Flush Cache ---
        // Crucial step after writing!
        outb(IDE_COMMAND_REG, IDE_CMD_FLUSH_CACHE);
        // Wait for the flush to complete (poll until BSY clear)
        poll_result = ide_poll_busy_clear();
        if (poll_result < 0) {
             term_writestring("IDE Write: Timeout polling after FLUSH CACHE.\n");
             return -1;
        }
        // Check for errors after flush
        if (poll_result & (IDE_STATUS_ERR | IDE_STATUS_DF)) {
             term_writestring("Error: IDE ERR/DF set after FLUSH CACHE.\n");
             return -6;
        }

        // Move to next buffer position
        current_buf_ptr += 512;
     }
     return 0; // Success
}