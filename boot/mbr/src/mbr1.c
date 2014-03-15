/**
 * MBR
 *
 * MBRのパーティションテーブルからアクティブなパーティションを探して、
 * そのパーティションの先頭１セクタ（VBR）をメモリ0x07C00に読み込む
 */

__asm__ (".code16gcc");


#include <stdint.h>


// MBRのパーティションテーブルが格納されている位置
#define PARTITION_TABLE_ADDR  (0x7C00 + 0x01BE)

#define ACTIVE_PARTITION  (0x80)

/* ディスク読み込み先セグメント */
#define DST_SEG   (0x07C0)


// 実際のパーティションテーブルの構造と同じ
struct REAL_PARTITION_TABLE_ENTRY {
    uint8_t active_flag;  // アクティブなら0x80。アクティブでないなら0
    uint8_t chs_start1;   // ヘッド(7-0)
    uint8_t chs_start2;   // シリンダの上位2ビット(7-6), セクタ(5-0)
    uint8_t chs_start3;   // シリンダの下位8ビット(7-0)
    uint8_t type;         // 0なら空
    uint8_t chs_end1;     // 開始と同じ構造
    uint8_t chs_end2;
    uint8_t chs_end3;
    uint32_t lba_start;   // リトルインディアン
    uint32_t lba_end;
} __attribute__ ((__packed__));


// パーティションテーブルの内容を解釈したもの
struct PARTITION_TABLE_ENTRY {
    int start_cylinder;
    int start_head;
    int start_sector;
};


static int get_active_partition(struct PARTITION_TABLE_ENTRY *ent);
static int load_1sector(int drive_no, char cyl_no, char head_no, char sec_no, uint16_t dst_seg);
static void print(const char *s);
//static void print_roman(int num);
static void forever_hlt(void);


/*
 * アクティブパーティションの先頭１セクタ（VBR）をメモリアドレス0x7C00に読み込む
 */
void main(uint16_t dx)
{
    int drive_no = dx & 0xFF;

    struct PARTITION_TABLE_ENTRY ent;
    if (get_active_partition(&ent) < 0) {
        print("[MBR] not found active partition");
        forever_hlt();
    }

    if (load_1sector(drive_no, ent.start_cylinder, ent.start_head, ent.start_sector, DST_SEG) != 0) {
        print("[MBR] could not load VBR");
        forever_hlt();
    }
}


static int get_active_partition(struct PARTITION_TABLE_ENTRY *ent)
{
    struct REAL_PARTITION_TABLE_ENTRY *tbl = (struct REAL_PARTITION_TABLE_ENTRY *) PARTITION_TABLE_ADDR;

    for (int i = 0; i < 4; i++) {
        if (tbl->active_flag != ACTIVE_PARTITION) {
            continue;
        }

        ent->start_cylinder = ((tbl->chs_start2 & 0xC0) << 2) | tbl->chs_start3;
        ent->start_head     = tbl->chs_start1;
        ent->start_sector   = tbl->chs_start2 & 0x3F;

        return 0;
    }

    return -1;
}


/*
 * ハードディスクから１セクタ分メモリに読み込む
 */
static int load_1sector(int drive_no, char cyl_no, char head_no, char sec_no, uint16_t dst_seg)
{
    char ret;

    /**
     * INT 0x13 / AH=0x02
     *
     * 引数:
     * - AL = 読み込みセクタ数
     * - CH = シリンダ番号
     * - CL = セクタ番号
     * - DH = ヘッド番号
     * - DL = ドライブ番号
     * - ES:BX = 読み込み先アドレス
     *
     * 戻り値:
     * - AH = 状態
     *
     * http://www.ctyme.com/intr/rb-0607.htm
     */
    __asm__ __volatile__ (
        "mov %1, %%es\n"
        "int $0x13"

        : "=a" (ret)
        : "r" (dst_seg), "0" (0x0201), "b" (0x0000),
          "c" ((cyl_no << 8) | sec_no), "d" ((head_no << 8) | drive_no)
    );

    return ret & 0xFF00;
}


/*
 * 文字列を出力する
 */
static void print(const char *s)
{
    /**
     * INT 0x10 / AH=0x0E
     *
     * 引数:
     * - AL = 文字コード
     * - BH = ページ番号
     * - BL = 色コード
     *
     * http://www.ctyme.com/intr/rb-0106.htm
     */
    while (*s) {
        __asm__ __volatile__ (
            "int $0x10"
 
            :
            : "a" (0x0E00 | *s), "b" (0x000F)
        );

        s++;
    }
}


/*
// デバッグ用数値出力。ローマ数字風に数字を出す。
static void print_roman(int num)
{
    while (num > 0) {
        if (num >= 1000) {
            print("M");
            num -= 1000;
        } else if (num >= 100) {
            print("C");
            num -= 100;
        } else if (num >= 10) {
            print("X");
            num -= 10;
        } else {
            print("I");
            num -= 1;
        }
    }
}
*/


static void forever_hlt(void)
{
    for (;;) {
        __asm__ __volatile__ ("hlt");
    }
}

