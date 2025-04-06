/* kernel/linker.ld */
ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

KERNEL_VIRTUAL_BASE = 0xC0100000; /* Example higher-half base (Optional advanced concept) */
KERNEL_PHYSICAL_BASE = 0x1000;   /* IMPORTANT: Physical load address */

SECTIONS
{
    /* Load kernel at KERNEL_PHYSICAL_BASE */
    . = KERNEL_PHYSICAL_BASE;

    .text : /* AT(ADDR) specifies the load address */
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(COMMON)
        *(.bss)
    }
    end = .; /* Symbol marking the end of the kernel image */
}