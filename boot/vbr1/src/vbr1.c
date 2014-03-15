/**
 * VBR
 *
 * ディスクからVBR以降の内容をメモリに読み込む
 *
 * ## ディスク読み込み時の注意
 * (このプログラムの場合は1セクタずつ読み込んでいるので気にしなくてよい)
 * - 複数のトラックに、またがって読み込めない
 * - 64KB境界をまたごうとするとエラーになる(エラーコード %AH=0x09)
 *   つまり0x10000, 0x20000, ... をまたいで読み込めない
 */

__asm__ (".code16gcc");


#include <stdbool.h>
#include <stdint.h>


/* パーティション開始位置（VBRは除く） */
#define START_CYLINDER  0
#define START_HEAD      0
#define START_SECTOR    3

/* 読み込むセクタ数。0x07E00 - 0xA0000 に収まること。VBRを除き最大1217 */
#define LOAD_SECTORS   1200

/* ディスク読み込み先セグメント */
#define DST_SEG   (0x07E0)

/* １セクタのサイズ（単位はバイト） */
#define SECTOR_BYTES  512


static void get_drive_parameters(int drive_no, int *num_heads, int *num_sectors);
static int load_1sector(int drive_no, char cyl_no, char head_no, char sec_no, uint16_t dst_seg);
static void print(const char *s);
//static void print_roman(int num);
static void forever_hlt(void);


void main(uint16_t dx)
{
    int drive_no = dx & 0xFF;

    int num_heads, num_sectors;
    get_drive_parameters(drive_no, &num_heads, &num_sectors);

    bool first_time = true;
    short dst_seg = DST_SEG;
    char cyl_no, head_no, sec_no;
    int loaded_sectors = 0;

    for (cyl_no = START_CYLINDER; cyl_no < 0x03FF; cyl_no++) {
        for (head_no = (first_time) ? START_HEAD : 0; head_no < num_heads; head_no++) {
            // 注意: セクタ番号だけ 1 から始まる
            for (sec_no = (first_time) ? START_SECTOR : 1; sec_no <= num_sectors; sec_no++) {
                first_time = false;

                if (load_1sector(drive_no, cyl_no, head_no, sec_no, dst_seg) != 0) {
                    print("Load Error!");
                    forever_hlt();
                }

                // 読み込み先アドレスを１セクタ分（512 Bytes）進めるが
                // セグメントレジスタなので 1/16 しておく
                dst_seg += SECTOR_BYTES >> 4;

                if (++loaded_sectors >= LOAD_SECTORS) {
                    return;
                }
            }
        }
    }
}


static void get_drive_parameters(int drive_no, int *num_heads, int *num_sectors)
{
    unsigned int dx, cx;
    unsigned char carry_flag;

    /**
     * INT 0x13 / AH=0x08
     *
     * 引数:
     * - DL = ドライブ番号
     * - ES:DI = BIOSのバグから守るため両方0にしておく
     *
     * 戻り値:
     * - CF = エラーならON
     * - DH = ヘッド数 - 1
     * - CH = シリンダ数の下位8ビット
     * - CL = 1トラックあたりのセクタ数 (bits 5-0), シリンダ数の上位2ビット (bits 7-6)
     *
     * http://www.ctyme.com/intr/rb-0621.htm
     */
    __asm__ __volatile__ ("mov %0, %%es" : : "r" (0));
    __asm__ __volatile__ (
        "int $0x13\n"
        "setc %0"

        : "=r" (carry_flag), "=d" (dx), "=c" (cx)
        : "a" (0x0800), "1" (drive_no), "D" (0)
    );

    *num_heads = ((dx >> 8) & 0xFF) + 1;
    *num_sectors = cx & 0x3F;

    if (carry_flag) {
        print("Could not get drive parameters");
        forever_hlt();
    }

    /*
    print("heads: ");
    print_roman(*num_heads);
    print("    sectors: ");
    print_roman(*num_sectors);
    forever_hlt();
    */
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

