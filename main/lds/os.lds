OUTPUT_FORMAT("binary");

SECTIONS
{
    . = 0x280000;

    .head : {
        LONG(64 * 1024)         /*  0 : stack+.data+heap の大きさ（4KBの倍数） */
        LONG(0x69726148)        /*  4 : シグネチャ "Hari" */
        LONG(0)                 /*  8 : mmarea の大きさ（4KBの倍数） */
        LONG(0x310000)          /* 12 : スタック初期値＆.data転送先 */
        LONG(SIZEOF(.data))     /* 16 : .dataサイズ */
        LONG(LOADADDR(.data))   /* 20 : .dataの初期値列のファイル位置 */
        LONG(0xE9000000)        /* 24 : 0xE9000000 */
        LONG(OnSenMain - 0x280000 - 0x20)  /* 28 : エントリアドレス - 0x20 */
        LONG(0)                 /* 32 : heap領域（malloc領域）開始アドレス */
    }

    .text : {
        * (.text)
    }

    .data : {
        *(.data)
        *(.rodata*)
        *(.bss)
    }

    .eh_frame : {
        *(.eh_frame)
    }

}

