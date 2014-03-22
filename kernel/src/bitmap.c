/**
 * ビットマップ（24ビットのみ対応）
 *
 * ## BMP ファイルの構造
 * - ファイルヘッダ(BMP_FILE_HDR)
 * - 情報ヘッダ(BMP_INFO_HDR)
 * - 画像データ
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_BITMAP
#define HEADER_BITMAP

int load_bmp(void *p, unsigned int size_B);

#endif


//=============================================================================
// 非公開ヘッダ

#include <stdbool.h>

#include "debug.h"
#include "graphic.h"
#include "memory.h"


#define TYPE_BMP  ('B' | ('M' << 8))


static bool load_header(void *p, unsigned int size_B, int *w, int *h,
        int *pad_B);
static int load_bmp_rgb24(unsigned char *p_file, int size_B, int w, int h,
        int pad_B);


// ファイルヘッダ
struct BMP_FILE_HDR {
    unsigned short type;         // ファイルタイプ。'BM'
    unsigned long size_B;        // ファイルサイズ
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned long offbits;       // ファイル先頭から画像データまでのオフセット
} __attribute__ ((__packed__));


// 情報ヘッダ
struct BMP_INFO_HDR {
    unsigned long size_B;        // 情報ヘッダのサイズ
    long width_px;               // 画像の幅
    long height_px;              // 画像の高さ
    unsigned short planes;       // プレーン数。常に１
    unsigned short bits;         // １画素あたりのデータサイズ(1,4,8,24,32)
    // 以下0ならデフォルト値が使用される
    unsigned long compression;   // 圧縮形式。BMPなら0（無圧縮）
    unsigned long size_img_B;    // 画像データ部のサイズ。BMPなら0
    long x_px_per_meter;         // 横方向解像度
    long y_px_per_meter;         // 縦方向解像度
    unsigned long color_used;    // 格納されているパレット数（使用色数）
    unsigned long clr_important; // 重要なパレットのインデックス
} __attribute__ ((__packed__));



//=============================================================================
// 公開関数

int load_bmp(void *p, unsigned int size_B)
{
    int w, h, pad_B;

    if ( ! load_header(p, size_B, &w, &h, &pad_B)) {
        return ERROR_SID;
    }

    return load_bmp_rgb24(p, size_B, w, h, pad_B);
}


//=============================================================================
// 非公開関数

static bool load_header(void *p, unsigned int size_B, int *w, int *h,
        int *pad_B)
{
    // ---- ファイルヘッダ

    // サイズチェック
    int cur_size_B = sizeof (struct BMP_FILE_HDR) + sizeof (struct BMP_INFO_HDR);
    if (size_B <= cur_size_B) {
        DBGF("file size error");
        return false;
    }

    struct BMP_FILE_HDR *f_hdr = (struct BMP_FILE_HDR *) p;

    if (f_hdr->type != TYPE_BMP) {
        DBGF("[header] type error");
        return false;
    }


    // ---- 情報ヘッダ

    struct BMP_INFO_HDR *i_hdr = (struct BMP_INFO_HDR *) (f_hdr + 1);

    if (i_hdr->size_B != sizeof (struct BMP_INFO_HDR)) {
        DBGF("[header] info size error");
        dbgf("expect: %d\n", sizeof(struct BMP_INFO_HDR));
        dbgf("passed: %d\n", i_hdr->size_B);

        return false;
    }

    *w = i_hdr->width_px;
    *h = i_hdr->height_px;

    if (i_hdr->bits != 24) {
        DBGF("bits is not 24");
        return false;
    }

    // 画像データ１行のバイト数
    // (... + 7) / 8 の部分は切り上げ
    int line_B = (i_hdr->bits * *w + 7) / 8;

    // パディング（４バイト境界に揃えるため）
    *pad_B = 4 - (line_B % 4);
    *pad_B = (*pad_B == 4) ? 0 : *pad_B;


    // ---- 画像データ

    // サイズチェック
    cur_size_B += (line_B + *pad_B) * *h;
    if (size_B < cur_size_B) {
        return false;
    }

    return true;
}


static int load_bmp_rgb24(unsigned char *p_file, int size_B, int w, int h,
        int pad_B)
{
    // - 画像データは左下から右上に向かって記録されている
    // - 画像の横ラインのデータは4バイトの境界に揃えないといけない

    COLOR16 *buf = mem_alloc(w * h * sizeof (COLOR16));

    if (buf == 0) {
        return ERROR_SID;
    }

    int sid = new_surface_from_buf(NO_PARENT_SID, w, h, buf, 16);
    if (sid == ERROR_SID) {
        return sid;
    }

    unsigned char *p = p_file + sizeof (struct BMP_FILE_HDR) + sizeof (struct BMP_INFO_HDR);
    unsigned char r, g, b;

    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            b = *p++;
            g = *p++;
            r = *p++;

            buf[y * w + x] = RGB16(r, g, b);
        }

        p += pad_B;
    }

    return sid;
}

