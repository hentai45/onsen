ENTRY(OnSenMain)

_OS_BASE = 0xC0100000;

SECTIONS
{
    . = _OS_BASE;

    .text : {
        *(.text)
        *(.rodata*)
        *(.eh_frame)
    }

    .data : ALIGN(0x1000) {
        *(.data)
        *(.bss)
    }
}

