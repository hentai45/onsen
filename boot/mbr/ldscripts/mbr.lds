/* OUTPUT_FORMAT("binary"); */
MBR_BASE = 0x0500;

SECTIONS
{
    . = MBR_BASE;

    .text   : {*(.text)}
    .rodata : {*(.rodata*)}
    .data   : {*(.data)}
    .bss    : {*(.bss)}

    .partition_table1 : AT(MBR_BASE + 0x01BE) {
        BYTE(0x80)  /* active */
        BYTE(0) BYTE(2) BYTE(0)  /* start CHS sector: c=0, h=0, s=2 */
        BYTE(0x20)               /* partition type: original */
        BYTE(0) BYTE(0) BYTE(0)  /* end CHS sector */
        LONG(0)                  /* start LBA sector */
        LONG(0)                  /* partition size */
    }

    .partition_table2 : AT(MBR_BASE + 0x01CE) {
        BYTE(0)  /* not active */
        BYTE(0) BYTE(0) BYTE(0)  /* start CHS sector */
        BYTE(0)                  /* partition type: empty */
        BYTE(0) BYTE(0) BYTE(0)  /* end CHS sector */
        LONG(0)                  /* start LBA sector */
        LONG(0)                  /* partition size */
    }

    .partition_table3 : AT(MBR_BASE + 0x01DE) {
        BYTE(0)                  /* not active */
        BYTE(0) BYTE(0) BYTE(0)  /* start CHS sector */
        BYTE(0)                  /* partition type: empty */
        BYTE(0) BYTE(0) BYTE(0)  /* end CHS sector */
        LONG(0)                  /* start LBA sector */
        LONG(0)                  /* partition size */
    }

    .partition_table4 : AT(MBR_BASE + 0x01EE) {
        BYTE(0)                  /* not active */
        BYTE(0) BYTE(0) BYTE(0)  /* start CHS sector */
        BYTE(0)                  /* partition type: empty */
        BYTE(0) BYTE(0) BYTE(0)  /* end CHS sector */
        LONG(0)                  /* start LBA sector */
        LONG(0)                  /* partition size */
    }

    .signature : AT(MBR_BASE + 0x01FE) {
        SHORT(0xAA55)
    }

    /DISCARD/ : {*(.eh_frame)}
}

