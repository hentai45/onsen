/**
 * グラフィック用バッファ
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_GBUFFER
#define HEADER_GBUFFER

#include <stdbool.h>
#include "graphic.h"

#define GBF_FLG_HAS_COLORKEY    (1 << 0)
#define GBF_FLG_HAS_ALPHA       (1 << 1)

struct _GBUFFER_METHOD;

typedef struct _GBUFFER {
    void *buf;

    int w;
    int h;
    int color_width;

    unsigned int flags;

    COLOR32 colorkey;  // 透明にする色。SRF_FLG_HAS_COLORKEYで有効。
    unsigned char alpha;

    struct _GBUFFER_METHOD *m;
} GBUFFER;


typedef struct _GBUFFER_METHOD {
    COLOR32 (*get)(GBUFFER *self, int x, int y);
    void (*put)(GBUFFER *self, int x, int y, COLOR32 color);
    void (*fill_rect)(GBUFFER *self, int x, int y, int w, int h, COLOR32 color);
    void (*blit)(GBUFFER *self, int src_x, int src_y, int w, int h, GBUFFER *dst, int x, int y, int op);
} GBUFFER_METHOD;


extern struct _GBUFFER_METHOD *g_gbuf_method16;

#endif


//=============================================================================
// 非公開ヘッダ

#include "debug.h"
#include "stacktrace.h"
#include "str.h"


#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAXMIN(A, B, C) MAX(A, MIN(B, C))
#define ABS(X) ((X) < 0 ? -(X) : X)


static COLOR32 get16(GBUFFER *self, int x, int y);
static void put16(GBUFFER *self, int x, int y, COLOR32 color);
static void fill_rect16(GBUFFER *self, int x, int y, int w, int h, COLOR32 color);
static void blit16(GBUFFER *self, int src_x, int src_y, int w, int h, GBUFFER *dst, int dst_x, int dst_y, int op);

static struct _GBUFFER_METHOD l_gbuf_method16 = { .get = get16, .put = put16,
    .fill_rect = fill_rect16, .blit = blit16 };
struct _GBUFFER_METHOD *g_gbuf_method16 = &l_gbuf_method16;



//=============================================================================
// 関数

static COLOR32 get16(GBUFFER *self, int x, int y)
{
    if (x < 0 || y < 0 || self->w <= x || self->h <= y)
        return 0;

    COLOR16 *buf = (COLOR16 *) self->buf;
    return RGB16_TO_32(buf[y * self->w + x]);
}


static void put16(GBUFFER *self, int x, int y, COLOR32 color)
{
    if (x < 0 || y < 0 || self->w <= x || self->h <= y)
        return;

    COLOR16 *buf = (COLOR16 *) self->buf;
    buf[y * self->w + x] = RGB32_TO_16(color);
}


// 矩形を塗りつぶす
static void fill_rect16(GBUFFER *self, int x, int y, int w, int h, COLOR32 color)
{
    if (w == 0 && h == 0) {
        w = self->w;
        h = self->h;
    }

    x = MAXMIN(0, x, self->w - 1);
    y = MAXMIN(0, y, self->h - 1);

    int max_w = self->w - x;
    int max_h = self->h - y;

    w = MAXMIN(0, w, max_w);
    h = MAXMIN(0, h, max_h);

    COLOR16 *buf = (COLOR16 *) self->buf;
    buf = &buf[y * self->w + x];
    COLOR16 *buf1 = buf;  // 1行目

    COLOR16 col = RGB32_TO_16(color);

    /* 1行目を塗りつぶす */
    for (int px = 0; px < w; px++) {
        buf1[px] = col;
    }
    buf += self->w;

    /* 2行目以降は1行目のコピー */
    for (int line = 1; line < h; line++) {
        memcpy(buf, buf1, w * sizeof (COLOR16));
        buf += self->w;
    }
}

static void blit_src_copy16(GBUFFER *src, int src_x, int src_y, int w, int h,
        GBUFFER *dst, int dst_x, int dst_y);
static void blit_src_invert16(GBUFFER *src, int src_x, int src_y, int w, int h,
        GBUFFER *dst, int dst_x, int dst_y);

static void blit16(GBUFFER *self, int src_x, int src_y, int w, int h,
        GBUFFER *dst, int dst_x, int dst_y, int op)
{
    // ---- 範囲チェック＆修正

    int src_max_w = self->w - src_x;
    int src_max_h = self->h - src_y;
    int dst_max_w = dst->w  - dst_x;
    int dst_max_h = dst->h  - dst_y;

    src_x = MAXMIN(0, src_x, self->w - 1);
    src_y = MAXMIN(0, src_y, self->h - 1);
    dst_x = MAXMIN(0, dst_x, dst->w  - 1);
    dst_y = MAXMIN(0, dst_y, dst->h  - 1);

    if (self == dst && src_x == dst_x && src_y == dst_y) {
        return;
    }

    w = MIN(w, MIN(src_max_w, dst_max_w));
    h = MIN(h, MIN(src_max_h, dst_max_h));

    // ---- 本体

    switch (op) {
    case OP_SRC_COPY:
        blit_src_copy16(self, src_x, src_y, w, h, dst, dst_x, dst_y);
        break;

    case OP_SRC_INVERT:
        blit_src_invert16(self, src_x, src_y, w, h, dst, dst_x, dst_y);
        break;
    }
}


// ビットマップコピー
static void blit_src_copy16(GBUFFER *src, int src_x, int src_y, int w, int h,
        GBUFFER *dst, int dst_x, int dst_y)
{
    // src と dst が同じ場合は、重なっている場合も考慮しないといけないので
    // 場合分けが少し複雑になっている。

    COLOR16 *src_buf = (COLOR16 *) src->buf;
    src_buf = &src_buf[src_y * src->w + src_x];;

    COLOR16 *dst_buf = (COLOR16 *) dst->buf;
    dst_buf = &dst_buf[dst_y * dst->w + dst_x];

    if (src->flags & GBF_FLG_HAS_COLORKEY) {
        COLOR16 colorkey = RGB32_TO_16(src->colorkey);

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (*src_buf != colorkey) {
                    *dst_buf = *src_buf;
                }

                src_buf++;
                dst_buf++;
            }

            src_buf += (src->w - w);
            dst_buf += (dst->w - w);
        }
    } else if (src->flags & GBF_FLG_HAS_ALPHA) {
        unsigned char alpha = src->alpha;

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                *dst_buf = RGB16(
                        (GET_RED16(*src_buf) * (100 - alpha) +
                        GET_RED16(*dst_buf) * alpha) / 100,
                        (GET_GREEN16(*src_buf) * (100 - alpha) +
                        GET_GREEN16(*dst_buf) * alpha) / 100,
                        (GET_BLUE16(*src_buf) * (100 - alpha) +
                        GET_BLUE16(*dst_buf) * alpha) / 100);

                src_buf++;
                dst_buf++;
            }

            src_buf += (src->w - w);
            dst_buf += (dst->w - w);
        }
    } else {
        // srcとdstの幅が同じなら一気にコピー
        if (src->w == w && src->w == dst->w) {
            if (src == dst) {
                memmove(dst_buf, src_buf, w * h * sizeof (COLOR16));
            } else {
                memcpy(dst_buf, src_buf, w * h * sizeof (COLOR16));
            }
        } else {  // src と dst の幅が違うなら１行ずつコピー
            if (src == dst) {
                if (src_y >= dst_y) {
                    for (int y = 0; y < h; y++) {
                        memmove(dst_buf, src_buf, w * sizeof (COLOR16));

                        src_buf += src->w;
                        dst_buf += dst->w;
                    }
                } else {
                    for (int y = h; y >= 0; y--) {
                        memmove(dst_buf, src_buf, w * sizeof (COLOR16));

                        src_buf += src->w;
                        dst_buf += dst->w;
                    }
                }
            } else {
                for (int y = 0; y < h; y++) {
                    memcpy(dst_buf, src_buf, w * sizeof (COLOR16));

                    src_buf += src->w;
                    dst_buf += dst->w;
                }
            }
        }
    }
}


// ビットマップxor。srcとdstの重なりには対応してない
static void blit_src_invert16(GBUFFER *src, int src_x, int src_y, int w, int h,
        GBUFFER *dst, int dst_x, int dst_y)
{
    COLOR16 *src_buf = (COLOR16 *) src->buf;
    src_buf = &src_buf[src_y * src->w + src_x];;

    COLOR16 *dst_buf = (COLOR16 *) dst->buf;
    dst_buf = &dst_buf[dst_y * dst->w + dst_x];

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            *dst_buf ^= *src_buf;

            src_buf++;
            dst_buf++;
        }

        src_buf += (src->w - w);
        dst_buf += (dst->w - w);
    }
}

