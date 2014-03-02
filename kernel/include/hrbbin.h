#ifndef HRBBIN_H
#define HRBBIN_H

typedef struct _HRB_HEADER {
    long stack_and_data_size;   /*  0 : stack+.data+heap の大きさ（4KBの倍数） */
    long signature;             /*  4 : シグネチャ "Hari" */
    long bss_size;              /*  8 : bss の大きさ（4KBの倍数） */
    long dst_data;              /* 12 : スタック初期値＆.data転送先 */
    long data_size;             /* 16 : .dataサイズ */
    long src_data;              /* 20 : .dataの初期値列のファイル位置 */
    long jmp;                   /* 24 : 0xE9000000 */
    long entry;                 /* 28 : エントリアドレス - 0x20 */
    long addr_heap;             /* 32 : heap領域（malloc領域）開始アドレス */
} HRB_HEADER;

#endif
