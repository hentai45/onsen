/* OUTPUT_FORMAT("binary"); */
IPLBASE = 0x7C00;

SECTIONS
{
    . = IPLBASE;

    .text   :   {*(.text)}
    .rodata :   {*(.rodata*)}
    .data   :   {*(.data)}
    .bss    :   {*(.bss)}

    .signature : AT(IPLBASE + 510) {
        SHORT(0xAA55)
    }

    /DISCARD/ : {*(.eh_frame)}
}

