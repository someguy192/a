; kernel/start.asm
; NASM syntax
; Entry point for the kernel, called by the bootloader AFTER protected mode is enabled.

bits 32          ; We are now in 32-bit Protected Mode

section .text
global _start    ; Export _start symbol for the linker (entry point)
extern kernel_main ; Declare external C function

_start:
    ; The bootloader already set up GDT, segments (DS, ES, FS, GS, SS)
    ; and a basic stack pointer (ESP).
    ; We can now directly call our C kernel main function.

    call kernel_main

    ; Halt the system if kernel_main returns (it shouldn't)
    cli ; Disable interrupts
.hang:
    hlt ; Halt the CPU
    jmp .hang ; Loop indefinitely


; We still need a .bss section for the C stack, even though ESP was initially set by bootloader
section .bss
align 16
stack_bottom:
resb 16384 ; 16 KiB stack
stack_top:

; Note: Although ESP is set by the bootloader initially (e.g., to 0x90000),
; the C environment typically expects the stack to be within its BSS.
; A more robust kernel entry would explicitly set ESP to 'stack_top' here.
; However, for simplicity, we rely on the bootloader's ESP setting being usable
; and the C code using the stack space defined here indirectly.
; For a production system, you'd set ESP = stack_top here.
; Example: mov esp, stack_top