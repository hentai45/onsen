OUTPUT_FORMAT("binary");
IPLBASE = 0x7C00;

SECTIONS
{
    . = IPLBASE;
    .text   : { *(.text) }
    .data   : { *(.data) }
    . = IPLBASE + 510;
    .sign   : { SHORT(0xAA55) }
}

