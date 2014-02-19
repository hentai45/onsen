/**
 * 画面設定
 * http://community.osdev.info/?VESA
 */

__asm__ (".code16gcc\n");

#include "sysinfo.h"

#define VBE_640x480_16bit   (0x0111)

/***** モード *****/
#define VBE_MODE VBE_640x480_16bit

#if VBE_MODE == VBE_640x480_16bit
#define COLOR_WIDTH    (16)
#define MEMORY_PLANES  (6)
#endif


/* SuperVGA 情報 */
typedef struct _SVGA_INFO {
    unsigned long  signature;   /* シグネチャ 'VESA' */
    unsigned short version;     /* VESAのバージョン（例：1.02なら0x0102） */
    unsigned long  p_oem_name;  /* OEM名ストリングへのポインタ(ASCIZ) */
    unsigned long  flags;       /* 各種フラグ */
    unsigned long  p_mode_list; /* ビデオモード列挙配列へのポインタ、word配列、終端は0xffff */
    unsigned short vram_size;   /* VESAがサポートするVRAM容量 */
    char  reserved[236];        /* 予約 */
} __attribute__ ((__packed__)) SVGA_INFO;


/* SuperVGA モード情報 */
typedef struct _SVGA_MODE_INFO {
    unsigned short mode_attr;   /* モード属性 */
    char skip0[16];
    unsigned short w;           /* Xの解像度 */
    unsigned short h;           /* Yの解像度 */
    char skip1[3];
    unsigned char  color_width; /* 1ピクセルが何ビットか（要するに色数） */
    char skip2[1];
    unsigned char  memory_planes; /* メモリモデル（0x04=256パレット、0x06=ダイレクトカラー） */
    char skip3[12];
    unsigned long  vram;        /* VRAMベースアドレス */
    char skip4[212];
} __attribute__ ((__packed__)) SVGA_MODE_INFO;


static int get_svga_info(SVGA_INFO *svga);
static int get_svga_mode_info(SVGA_MODE_INFO *svga_mode);
static void set_svga_mode(void);
static void screen320x200(void);

/**
 * 画面モードを設定する
 */
void set_video_mode(void)
{
    SVGA_INFO svga;
    SVGA_MODE_INFO svga_mode;

    if (get_svga_info(&svga) < 0) {
        screen320x200();
        return;
    }

    /* バージョン2以上が必要 */
    if (svga.version < 0x0200) {
        screen320x200();
        return;
    }

    if (get_svga_mode_info(&svga_mode) < 0) {
        screen320x200();
        return;
    }

    set_svga_mode();

    g_sys_info->vram = svga_mode.vram;
    g_sys_info->w = svga_mode.w;
    g_sys_info->h = svga_mode.h;
}


/**
 * SuperVGA 情報を取得する
 * @return 0 = 成功, -1 = 失敗
 */
static int get_svga_info(SVGA_INFO *svga)
{
    unsigned short status;
    unsigned long svga_long = (unsigned long) svga;
    unsigned short svga_segment = ((svga_long >> 4) & 0xFFFF);
    unsigned short svga_offset  = svga_long & 0xF;

    /**
     * INT 0x10 / AX=0x4F00
     *
     * 引数:
     * - ES:DI = SuperVGA情報を書き込む
     *
     * 戻り値:
     * - AL = 関数がサポートされているなら0x4F
     *
     * http://www.ctyme.com/intr/rb-0273.htm
     */
    __asm__ __volatile__ (
        "movw %1, %%bx\n"
        "movw %%bx, %%es\n"
        "int $0x10\n"
 
        : "=a"(status)
        : "r"(svga_segment),
          "0"(0x4F00),
          "D"(svga_offset)
        : "%bx"
    );

    if (status & 0x004F) {
        return 0;
    } else {
        return -1;
    }
}


/**
 * SuperVGA モード情報を取得する
 * @return 0 = 成功, -1 = 失敗
 */
static int get_svga_mode_info(SVGA_MODE_INFO *svga_mode)
{
    unsigned short status;
    unsigned long svga_long = (unsigned long) svga_mode;
    unsigned short svga_segment = ((svga_long >> 4) & 0xFFFF);
    unsigned short svga_offset  = svga_long & 0xF;

    /**
     * INT 0x10 / AX=0x4F01
     *
     * 引数:
     * - CX = SuverVGA モード
     * - ES:DI = SuperVGA モード情報のアドレス
     *
     * 戻り値:
     * - AL = 関数がサポートされているなら0x4F
     *
     * http://www.ctyme.com/intr/rb-0274.htm
     */
    __asm__ __volatile__ (
        "movw %1, %%bx\n"
        "movw %%bx, %%es\n"
        "int $0x10\n"
 
        : "=a"(status)
        : "r"(svga_segment),
          "0"(0x4F01),
          "c"(VBE_MODE),
          "D"(svga_offset)
        : "%bx"
    );

    status &= 0x004F;

    if (status == 0) {  /* エラー */
        return -1;
    }

    if (svga_mode->color_width != COLOR_WIDTH) {
        screen320x200();
        return -1;
    }

    if (svga_mode->memory_planes != MEMORY_PLANES) {
        screen320x200();
        return -1;
    }

    /* リニアフレームバッファモードがサポートされているか？ */
    if ((svga_mode->mode_attr & 0x0080) == 0) {
        screen320x200();
        return -1;
    }

    return 0;
}


/**
 * 画面モードを切り替える
 */
static void set_svga_mode(void)
{
    int status;

    /**
     * INT 0x10 / AX=0x4F02
     *
     * 引数:
     * - BX = SuverVGA モード
     *
     * 戻り値:
     * - AL = 関数がサポートされているなら0x4F
     *
     * http://www.ctyme.com/intr/rb-0275.htm
     */
    __asm__ __volatile__ (
        "int $0x10\n"
 
        : "=a"(status)
        : "0"(0x4F02),
          "b"(0x4000 | VBE_MODE)
    );

    status &= 0x004F;
    if (status == 0) {  /* エラー */
        __asm__ __volatile__ ("hlt"); // TODO
    }
}


static void screen320x200(void)
{
    __asm__ __volatile__ ("hlt"); // TODO
}
