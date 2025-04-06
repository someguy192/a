; boot/boot.asm
; A very basic 16-bit bootloader that loads a kernel and enters 32-bit protected mode.
; NASM syntax

bits 16         ; We start in 16-bit real mode
org 0x7c00      ; BIOS loads boot sectors here

KERNEL_LOAD_ADDR equ 0x1000 ; Address where we will load the kernel
KERNEL_SECTOR_START equ 2     ; Kernel starts at the 2nd sector (1st is bootloader)
KERNEL_SECTORS_TO_LOAD equ 20 ; How many sectors to load (adjust if kernel grows)

start:
    ; --- Initial Setup ---
    cli             ; Disable interrupts during setup
    xor ax, ax      ; AX = 0
    mov ds, ax      ; Set DS=0 (important for BIOS calls)
    mov es, ax      ; Set ES=0
    mov ss, ax      ; Set SS=0
    mov sp, 0x7c00  ; Stack grows downwards from bootloader base

    sti             ; Re-enable interrupts for BIOS calls

    ; --- Print Boot Message (Optional) ---
    mov si, boot_msg
print_loop:
    lodsb           ; Load byte [SI] into AL, increment SI
    or al, al       ; Check if AL is zero (end of string)
    jz load_kernel  ; If zero, jump to kernel loading
    mov ah, 0x0e    ; BIOS teletype output function
    mov bh, 0x00    ; Page number 0
    mov bl, 0x07    ; Light grey text on black background
    int 0x10        ; Call BIOS video interrupt
    jmp print_loop

load_kernel:
    ; --- Load Kernel from Disk using BIOS int 13h ---
    ; Assumes boot drive is floppy (DL=0) or first hard disk (DL=0x80)
    ; BIOS stores boot drive number in DL, so we just use it.

    mov bx, KERNEL_LOAD_ADDR ; ES:BX = destination buffer (ES=0)
    mov ah, 0x02             ; BIOS Read Sectors function
    mov al, KERNEL_SECTORS_TO_LOAD ; Number of sectors to read
    mov ch, 0                ; Cylinder number (track) = 0
    mov cl, KERNEL_SECTOR_START ; Sector number to start reading from
    mov dh, 0                ; Head number = 0
    ; DL = drive number (already set by BIOS)

    int 0x13                 ; Call BIOS disk interrupt
    jc disk_error            ; Jump if carry flag set (error)

    ; --- Switch to Protected Mode ---
    cli                      ; Disable interrupts PERMANENTLY before mode switch

    ; 1. Load GDT
    lgdt [gdt_ptr]           ; Load Global Descriptor Table Register

    ; 2. Enable A20 Line (Basic Keyboard Controller Method)
    call enable_a20

    ; 3. Set PE bit in CR0
    mov eax, cr0             ; Get control register 0
    or eax, 0x1              ; Set bit 0 (Protection Enable)
    mov cr0, eax             ; Write back to control register 0

    ; 4. Far jump to flush prefetch queue and load CS with 32-bit selector
    jmp CODE_SEG:protected_mode_entry ; CODE_SEG = 0x08 (from GDT)

disk_error:
    mov si, disk_err_msg
    jmp print_loop ; Print error and hang (print_loop terminates on null)

enable_a20:
    ; Basic A20 gate enable using keyboard controller
    in al, 0x64     ; Read status port
    test al, 2      ; Check if input buffer full (bit 1)
    jnz enable_a20  ; Loop if busy

    mov al, 0xD1    ; Command: Write Output Port
    out 0x64, al    ; Send command

a20_wait_write:
    in al, 0x64     ; Read status port
    test al, 2      ; Check if input buffer full
    jnz a20_wait_write ; Loop if busy

    mov al, 0xDF    ; Data: Enable A20 (bit 1 set)
    out 0x60, al    ; Send data to data port
    ret

; --- Protected Mode Entry Point ---
bits 32                  ; Switch assembler to 32-bit mode
protected_mode_entry:
    ; Set up data segment registers for flat 32-bit mode
    mov ax, DATA_SEG     ; DATA_SEG = 0x10 (from GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax           ; Set stack segment

    ; Set up stack pointer somewhere safe (e.g., 0x90000)
    ; Note: Kernel's BSS hasn't been cleared yet.
    mov esp, 0x90000

    ; Jump to the loaded kernel code
    jmp KERNEL_LOAD_ADDR

; --- Global Descriptor Table (GDT) ---
gdt_start:
    ; Null Descriptor
    dd 0x0                  ; double word (4 bytes)
    dd 0x0

    ; Code Segment Descriptor (Base=0, Limit=4GB, 32-bit, Ring 0)
gdt_code:
    dw 0xFFFF               ; Limit (low bits)
    dw 0x0000               ; Base (low bits)
    db 0x00                 ; Base (mid bits)
    db 0b10011010           ; Access Byte: P=1, DPL=00, S=1, Type=1010 (Execute/Read, Non-conforming)
    db 0b11001111           ; Granularity Byte: G=1(4K pages), D/B=1(32-bit), L=0, AVL=0, Limit (high bits)
    db 0x00                 ; Base (high bits)

    ; Data Segment Descriptor (Base=0, Limit=4GB, 32-bit, Ring 0)
gdt_data:
    dw 0xFFFF               ; Limit (low bits)
    dw 0x0000               ; Base (low bits)
    db 0x00                 ; Base (mid bits)
    db 0b10010010           ; Access Byte: P=1, DPL=00, S=1, Type=0010 (Read/Write, Expand-up)
    db 0b11001111           ; Granularity Byte: G=1(4K pages), D/B=1(32-bit), L=0, AVL=0, Limit (high bits)
    db 0x00                 ; Base (high bits)
gdt_end:

; GDT Pointer Structure (Limit and Base)
gdt_ptr:
    dw gdt_end - gdt_start - 1 ; Limit (size of GDT - 1)
    dd gdt_start               ; Base address of GDT

; --- Segment Selectors (Offsets into GDT / 8) ---
CODE_SEG equ gdt_code - gdt_start ; Should be 0x08
DATA_SEG equ gdt_data - gdt_start ; Should be 0x10

; --- Data ---
boot_msg db "Booting MyOS...", 0x0d, 0x0a, 0
disk_err_msg db "Disk read error!", 0x0d, 0x0a, 0

; --- Boot Sector Padding and Signature ---
times 510 - ($-$$) db 0   ; Pad remainder of boot sector with 0s
dw 0xaa55                ; Boot sector magic number