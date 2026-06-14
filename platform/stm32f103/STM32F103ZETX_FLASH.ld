MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
  EXTSRAM(rwx): ORIGIN = 0x68000000, LENGTH = 1024K
}

_stack_top = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    .vectors : {
        KEEP(*(.vectors))
    } > FLASH

    .text : {
        KEEP(*(.init))
        *(.text*)
        *(.rodata*)
        KEEP(*(.fini))
        . = ALIGN(4);
    } > FLASH

    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH

    _etext = .;

    .data : AT(_etext) {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM

    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    .extsram (NOLOAD) : {
        *(.extsram*)
    } > EXTSRAM

    _end = .;
}
