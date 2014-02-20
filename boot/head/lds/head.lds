/* OUTPUT_FORMAT("binary"); */

SECTIONS
{
    . = 0xC200;

    .text   :   {*(.text)}
    .rodata :   {*(.rodata*)}
    .data   :   {*(.data)}
    .bss    :   {*(.bss)}

    OnSenMain = .;
}

