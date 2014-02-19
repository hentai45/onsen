/**
 * IPL
 */

__asm__ (".code16gcc\n");

/* ディスク読み込み先セグメント */
#define DST_SEG (0x0820)

/**
 * 読み込みシリンダ数
 * 33までなら安全に読み込める
 */
#define LOAD_CYLS (33)


/**
 * フロッピーディスク
 */

/* １セクタのサイズ（単位はバイト） */
#define FD_SECTOR_BYTES (512)

/* シリンダ数 */
#define FD_CYLS_NUM (80)

/* ヘッド数 */
#define FD_HEAD_NUM (2)

/* セクター数 */
#define FD_SECS_NUM (18)


#define MIN(x, y)    ((x) < (y) ? (x) : (y))


static int load_fd_1sector(char cyl_no, char head_no, char sec_no, short dst_seg);
static void print(const char *s);

/*
 * フロッピーディスクのデータをメモリに読み込む
 */
void load_fd(void)
{
    short dst_seg = DST_SEG;
    char cyl_no, head_no, sec_no;

    for (cyl_no = 0; cyl_no < MIN(LOAD_CYLS, FD_CYLS_NUM); cyl_no++) {
        for (head_no = 0; head_no < FD_HEAD_NUM; head_no++) {
            /* 注意: セクタ番号だけ 1 から始まる */
            for (sec_no = 1; sec_no <= FD_SECS_NUM; sec_no++) {
                /* ブートセクタは飛ばす */
                if (cyl_no == 0 && head_no == 0 && sec_no == 1) {
                    continue;
                }

                if (load_fd_1sector(cyl_no, head_no, sec_no, dst_seg) != 0) {
                    print("Load Error!");
                    for (;;) {
                        __asm__ __volatile__ ( "hlt\n" );
                    }
                }

                /* 読み込み先アドレスを１セクタ分（512 Bytes）進めるが
                 * セグメントレジスタなので 1/16 しておく */
                dst_seg += FD_SECTOR_BYTES >> 4;
            }
        }
    }
}


/*
 * フロッピーディスクから１セクタ分メモリに読み込む
 */
static int load_fd_1sector(char cyl_no, char head_no, char sec_no, short dst_seg)
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
        "movw %1, %%es\n"
        "int $0x13\n"

        : "=a"(ret)
        : "r"(dst_seg),
          "0"(0x0201),
          "b"(0x0000),
          "c"((cyl_no << 8) | sec_no),
          "d"(head_no << 8)
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
            "int $0x10\n"
 
            :
            : "a"(0x0E00 | *s),
              "b"(0x000F)
        );

        s++;
    }
}
