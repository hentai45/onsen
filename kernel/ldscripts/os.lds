ENTRY(OnSenMain)

_OS_BASE = 0xC0100000;

SECTIONS
{
    . = _OS_BASE;

    .text   :   {*(.text)}
    .rodata :   {*(.rodata*)}
    .eh_frame : {*(.eh_frame)}

    . = ALIGN (CONSTANT (MAXPAGESIZE)) - ((CONSTANT (MAXPAGESIZE) - .) & (CONSTANT (MAXPAGESIZE) - 1)); . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));

    .data   :   {*(.data)}
    .bss    :   {*(.bss)}
}

