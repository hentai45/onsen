/* OUTPUT_FORMAT("binary"); */

SECTIONS
{
    . = 0x7E00;

    .text   :   {*(.text)}
    .rodata :   {*(.rodata*)}
    .data   :   {*(.data)}
    .bss    :   {*(.bss)}

    .eh_frame : {*(.eh_frame)}

    OnSenElfStart = .;
}

