/**
 * グラフィック
 *
 * @file graphic.c
 * @author Ivan Ivanovich Ivanov
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_GRAPHIC
#define HEADER_GRAPHIC

#define ERROR_SID  -1

#define RGB(R, G, B) ((unsigned short) (((R) & 0xF8) << 8) | (((G) & 0xFC) << 3) | (((B) & 0xF8) >> 3))

#define GET_RED(RGB)   (((RGB) & 0xF800) >> 8)
#define GET_GREEN(RGB) (((RGB) & 0x07E0) >> 3)
#define GET_BLUE(RGB)  (((RGB) & 0x001F) << 3)

#define COL_BLACK    RGB(  0,   0,   0)
#define COL_RED      RGB(255,   0,   0)
#define COL_GREEN    RGB(  0, 255,   0)
#define COL_BLUE     RGB(  0,   0, 255)
#define COL_WHITE    RGB(255, 255, 255)


enum {
    OP_SRC_COPY,
    OP_SRC_INVERT
};


extern int g_vram_sid;
extern int g_dbg_sid;
extern int g_con_sid;
extern int g_world_sid;

extern const int g_w;
extern const int g_h;


void graphic_init(unsigned short *vram);

int  surface_new(int w, int h);
int  surface_new_from_buf(int w, int h, unsigned short *buf);
void surface_free(int sid);
void surface_task_free(int pid);
int  get_screen(void);

void set_sprite_pos(int sid, int x, int y);
void move_sprite(int sid, int dx, int dy);

void update_screen(int sid);
void update_rect(int sid, int x, int y, int w, int h);

void draw_sprite(int src_sid, int dst_sid, int op);
void blit_surface(int src_sid, int src_x, int src_y, int w, int h,
        int dst_sid, int dst_x, int dst_y, int op);

void fill_surface(int sid, unsigned short color);
void fill_rect(int sid, int x, int y, int w, int h,
        unsigned short color);
void draw_text(int sid, int x, int y, unsigned short color,
        const char *text);
void draw_text_bg(int sid, int x, int y, unsigned short color,
        unsigned short bg_color, const char *text);

void draw_pixel(int sid, unsigned int x, unsigned int y, unsigned short color);

void scroll_surface(int sid, int cx, int cy);

void set_colorkey(int sid, unsigned short colorkey);
void clear_colorkey(int sid);
void set_alpha(int sid, unsigned char alpha);
void clear_alpha(int sid);

void set_mouse_pos(int x, int y);

int get_screen_pid(void);
void switch_screen(void);
void switch_debug_screen(void);

void graphic_dbg(void);

#endif


//=============================================================================
// 非公開ヘッダ

#include "debug.h"
#include "graphic.h"
#include "memory.h"
#include "mouse.h"
#include "msg_q.h"
#include "str.h"
#include "task.h"


#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAXMIN(A, B, C) MAX(A, MIN(B, C))
#define ABS(X) ((X) < 0 ? -(X) : X)


#define SURFACE_MAX 1024

#define MOUSE_W  10
#define MOUSE_H  16

#define SRF_FLG_FREE            0
#define SRF_FLG_ALLOC           1
#define SRF_FLG_SCREEN          2
#define SRF_FLG_HAS_COLORKEY    4
#define SRF_FLG_HAS_ALPHA       8

#define HANKAKU_W 8   ///< 半角フォントの幅
#define HANKAKU_H 16  ///< 半角フォントの高さ


typedef struct SURFACE {
    int sid;    ///< SURFACE ID
    int flags;
    int pid;    ///< この SURFACE を持っているプロセス ID

    int x;
    int y;
    int w;
    int h;
    unsigned short *buf;

    unsigned short colorkey;  ///< 透明にする色。SRF_FLG_HAS_COLORKEYで透明。
    unsigned char alpha;

    struct SURFACE *prev_scr;
    struct SURFACE *next_scr;
} SURFACE;


typedef struct SURFACE_MNG {
    /// 記憶領域の確保用。
    /// surfaces のインデックスと SID は同じ
    SURFACE surfaces[SURFACE_MAX];

    /// 画面として使われている SURFACE の双方向循環リスト。
    /// head_scr が現在表示されている画面
    SURFACE *head_scr;
} SURFACE_MNG;


int g_vram_sid;  ///< この SID を使うと更新しなくても画面に直接表示される
int g_dbg_sid;
int g_con_sid;
int g_world_sid;

const int g_w = 640;  ///< 横の解像度
const int g_h = 480;  ///< 縦の解像度
const int g_d = 16;   // 色のビット数


static SURFACE_MNG l_mng;

static int l_mouse_sid;
static SURFACE *l_mouse_srf;


inline __attribute__ ((always_inline))
static SURFACE *sid2srf(int sid)
{
    if (sid < 0 || SURFACE_MAX <= sid) {
        return 0;
    }

    return &l_mng.surfaces[sid];
}


inline __attribute__ ((always_inline))
static int is_reserved_sid(int sid)
{
    return (sid == g_vram_sid || sid == g_dbg_sid || sid == g_con_sid ||
            sid == g_world_sid);
}


static SURFACE *srf_alloc(void);

static void create_mouse_surface(void);

static void blit_src_copy(SURFACE *src, int src_x, int src_y, int w, int h,
        SURFACE *dst, int dst_x, int dst_y);
static void blit_src_invert(SURFACE *src, int src_x, int src_y, int w, int h,
        SURFACE *dst, int dst_x, int dst_y);

static int draw_char(SURFACE *srf, int x, int y, unsigned short color, char ch);
static int draw_char_bg(SURFACE *srf, int x, int y, unsigned short color,
        unsigned short bg_color, char ch);

static void add_screen_head(SURFACE *srf);
static void add_screen_tail(SURFACE *srf);
static void remove_from_scr_list(SURFACE *srf);


//=============================================================================
// 公開関数

void graphic_init(unsigned short *vram)
{
    // ---- SURFACE 初期化

    for (int sid = 0; sid < SURFACE_MAX; sid++) {
        SURFACE *srf = &l_mng.surfaces[sid];

        srf->sid = sid;
        srf->flags = SRF_FLG_FREE;
        srf->pid = ERROR_PID;
    }


    // ---- SURFACE_MNG の初期化

    l_mng.head_scr = 0;


    // ---- SURFACE の作成

    // -------- VRAM SURFACE の作成

    SURFACE *srf = srf_alloc();
    int sid = srf->sid;

    g_vram_sid = sid;

    srf->w   = g_w;
    srf->h   = g_h;
    srf->buf = vram;

    // -------- デバッグ画面の作成

    sid = surface_new(g_w, g_h);
    g_dbg_sid = sid;
    srf = sid2srf(sid);
    srf->pid = g_dbg_pid;

    add_screen_head(srf);


    // -------- コンソール画面の作成

    sid = surface_new(g_w, g_h);
    g_con_sid = sid;
    srf = sid2srf(sid);
    srf->pid = g_con_pid;

    add_screen_head(srf);

    // -------- WORLD 画面の作成

    /*
    sid = surface_new(g_w, g_h);
    g_world_sid = sid;
    srf = sid2srf(sid);
    srf->pid = g_world_pid;

    add_screen_head(srf);
    */


    // -------- マウス SURFACE の作成

    create_mouse_surface();

}


/// 新しい SURFACE を作成する
int surface_new(int w, int h)
{
    SURFACE *srf = srf_alloc();

    if (srf == 0) {
        DBGF("could't create surface");
        return ERROR_SID;
    }

    int size = w * h * sizeof (short);
    unsigned short *buf = mem_alloc(size);

    srf->pid = get_pid();
    srf->x   = 0;
    srf->y   = 0;
    srf->w   = w;
    srf->h   = h;
    srf->buf = buf;

    return srf->sid;
}


/// @attention buf は mem_alloc で取得したメモリを使用すること
int surface_new_from_buf(int w, int h, unsigned short *buf)
{
    SURFACE *srf = srf_alloc();

    if (srf == 0) {
        DBGF("could't create surface");
        return ERROR_SID;
    }

    srf->pid = get_pid();
    srf->x   = 0;
    srf->y   = 0;
    srf->w   = w;
    srf->h   = h;
    srf->buf = buf;

    return srf->sid;
}


void surface_free(int sid)
{
    if (is_reserved_sid(sid)) {
        return;
    }

    SURFACE *srf = sid2srf(sid);

    if (srf == 0 || (srf->flags & SRF_FLG_ALLOC) == 0) {
        return;
    }

    if (srf->pid != get_pid()) {
        return;
    }

    if (srf->flags & SRF_FLG_SCREEN) {
        remove_from_scr_list(srf);
    }

    mem_free(srf->buf);
    srf->flags = SRF_FLG_FREE;
    srf->pid = ERROR_PID;
}


void surface_task_free(int pid)
{
    for (int sid = 0; sid < SURFACE_MAX; sid++) {
        SURFACE *srf = &(l_mng.surfaces[sid]);

        if (srf->pid != pid || (srf->flags & SRF_FLG_ALLOC) == 0) {
            continue;
        }

        if (srf->flags & SRF_FLG_SCREEN) {
            remove_from_scr_list(srf);
        }

        mem_free(srf->buf);
        srf->flags = SRF_FLG_FREE;
        srf->pid = ERROR_PID;
    }
}


int get_screen(void)
{
    int pid = get_pid();

    // ---- すでに作ってあればそれを返す

    SURFACE *srf = l_mng.head_scr;
    if (srf != 0) {
        do {
            if (srf->pid == pid) {
                return srf->sid;
            }

            srf = srf->next_scr;
        } while (srf != l_mng.head_scr);
    }


    // ---- 新規作成

    int sid = surface_new(g_w, g_h);
    srf = sid2srf(sid);

    // 前のアプリケーションの画面が残っているかもしれないので黒く塗りつぶす
    fill_surface(sid, COL_BLACK);

    // 画面をアプリケーションに切り替える
    add_screen_head(srf);
    update_screen(sid);

    return sid;
}


void set_sprite_pos(int sid, int x, int y)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0 || (srf->flags & SRF_FLG_SCREEN)) {
        return;
    }

    srf->x = MAXMIN(0, x, g_w);
    srf->y = MAXMIN(0, y, g_h);
}


void move_sprite(int sid, int dx, int dy)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0 || (srf->flags & SRF_FLG_SCREEN)) {
        return;
    }

    srf->x = MAXMIN(0, srf->x + dx, g_w);
    srf->y = MAXMIN(0, srf->y + dy, g_h);
}


void update_screen(int sid)
{
    update_rect(sid, 0, 0, 0, 0);
}


void update_rect(int sid, int x, int y, int w, int h)
{
    // いま表示されている画面以外の更新は無視する
    if (l_mng.head_scr->sid != sid) {
        return;
    }

    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (w == 0 && h == 0) {
        w = srf->w;
        h = srf->h;
    }

    blit_surface(sid, x, y, w, h, g_vram_sid, x, y, OP_SRC_COPY);


    int mx = l_mouse_srf->x;
    int my = l_mouse_srf->y;

    if (x <= mx && mx <= x + w && y <= my && my <= y + h) {
        // マウスカーソルを更新する
        blit_surface(l_mouse_sid, 0, 0, MOUSE_W, MOUSE_H,
                g_vram_sid, mx, my, OP_SRC_COPY);
    }
}


void draw_sprite(int src_sid, int dst_sid, int op)
{
    SURFACE *srf = sid2srf(src_sid);

    if (srf == 0) {
        return;
    }

    blit_surface(src_sid, 0, 0, srf->w, srf->h, dst_sid, srf->x, srf->y, op);
}


void blit_surface(int src_sid, int src_x, int src_y, int w, int h,
        int dst_sid, int dst_x, int dst_y, int op)
{
    SURFACE *src = sid2srf(src_sid);
    SURFACE *dst = sid2srf(dst_sid);

    if (src == 0 || dst == 0) {
        return;
    }

    // ---- 範囲チェック＆修正

    src_x = MAXMIN(0, src_x, src->w);
    src_y = MAXMIN(0, src_y, src->h);
    w = MAXMIN(0, w, src->w - src_x);
    h = MAXMIN(0, h, src->h - src_y);

    dst_x = MAXMIN(0, dst_x, dst->w);
    dst_y = MAXMIN(0, dst_y, dst->h);

    if (src == dst && src_x == dst_x && src_y == dst_y) {
        return;
    }

    w = MIN(w, dst->w - dst_x);
    h = MIN(h, dst->h - dst_y);


    // ---- 本体

    switch (op) {
    case OP_SRC_COPY:
        blit_src_copy(src, src_x, src_y, w, h, dst, dst_x, dst_y);
        break;

    case OP_SRC_INVERT:
        blit_src_invert(src, src_x, src_y, w, h, dst, dst_x, dst_y);
        break;
    }
}


/// 全体を塗りつぶす
void fill_surface(int sid, unsigned short color)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    fill_rect(sid, 0, 0, srf->w, srf->h, color);
}


/// 矩形を塗りつぶす
void fill_rect(int sid, int x, int y, int w, int h, unsigned short color)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    x = MAXMIN(0, x, srf->w);
    y = MAXMIN(0, y, srf->h);
    w = MAXMIN(0, w, srf->w - x);
    h = MAXMIN(0, h, srf->h - y);

    unsigned short *buf = srf->buf + (y * srf->w) + x;
    unsigned short *buf1 = buf;

    for (int px = 0; px < w; px++) {
        buf1[px] = color;
    }
    buf += srf->w;

    for (int line = 1; line < h; line++) {
        memcpy(buf, buf1, w * sizeof (short));
        buf += srf->w;
    }
}


/// 文字列を画面に出力する
void draw_text(int sid, int x, int y, unsigned short color, const char *s)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    for ( ; *s; s++) {
        x = draw_char(srf, x, y, color, *s);
    }
}


/// 文字列を画面に出力する（背景色を指定）
void draw_text_bg(int sid, int x, int y, unsigned short color,
        unsigned short bg_color, const char *s)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    for ( ; *s; s++) {
        x = draw_char_bg(srf, x, y, color, bg_color, *s);
    }
}


void draw_pixel(int sid, unsigned int x, unsigned int y, unsigned short color)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (x < srf->w && y <= srf->h) {
        srf->buf[srf->w * y + x] = color;
    }
}


void scroll_surface(int sid, int cx, int cy)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (ABS(cx) > srf->w || ABS(cy) > srf->h) {
        return;
    }

    if (cx == 0 && cy == 0) {
        return;
    }

    int src_x, dst_x;
    if (cx < 0) {
        src_x = -cx;
        dst_x = 0;
    } else {
        src_x = 0;
        dst_x = cx;
    }

    int src_y, dst_y;
    if (cy < 0) {
        src_y = -cy;
        dst_y = 0;
    } else {
        src_y = 0;
        dst_y = cy;
    }

    blit_surface(sid, src_x, src_y, srf->w, srf->h, sid, dst_x, dst_y,
            OP_SRC_COPY);
}


void set_colorkey(int sid, unsigned short colorkey)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    srf->flags &= ~SRF_FLG_HAS_ALPHA;
    srf->flags |= SRF_FLG_HAS_COLORKEY;
    srf->colorkey = colorkey;
}


void clear_colorkey(int sid)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    srf->flags &= ~SRF_FLG_HAS_COLORKEY;
}


void set_alpha(int sid, unsigned char alpha)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (alpha > 100) {
        alpha = 100;
    }

    srf->flags &= ~SRF_FLG_HAS_COLORKEY;
    srf->flags |= SRF_FLG_HAS_ALPHA;
    srf->alpha = alpha;
}


void clear_alpha(int sid)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    srf->flags &= ~SRF_FLG_HAS_ALPHA;
}


void set_mouse_pos(int x, int y)
{
    int mx = l_mouse_srf->x;
    int my = l_mouse_srf->y;

    blit_surface(l_mng.head_scr->sid, mx, my, MOUSE_W, MOUSE_H, g_vram_sid,
            mx, my, OP_SRC_COPY);

    l_mouse_srf->x = x;
    l_mouse_srf->y = y;

    blit_surface(l_mouse_sid, 0, 0, MOUSE_W, MOUSE_H, g_vram_sid,
            x, y, OP_SRC_COPY);
}


int get_screen_pid(void)
{
    return l_mng.head_scr->pid;
}


void switch_screen(void)
{
    // l_mng.head_scr が現在表示されている画面を表している。

    SURFACE *old_head = l_mng.head_scr;
    remove_from_scr_list(old_head);
    add_screen_tail(old_head);

    update_screen(l_mng.head_scr->sid);
}


void switch_debug_screen(void)
{
    SURFACE *dbg_srf = sid2srf(g_dbg_sid);
    remove_from_scr_list(dbg_srf);
    add_screen_head(dbg_srf);

    update_screen(l_mng.head_scr->sid);
}


void graphic_dbg(void)
{
    DBGF("DEBUG GRAPHIC");

    SURFACE *srf;

    dbgf("surfaces:\n");
    for (int sid = 0; sid < SURFACE_MAX; sid++) {
        srf = &l_mng.surfaces[sid];

        if (srf->flags & SRF_FLG_ALLOC) {
            dbgf("%d: pid = %d, buf = %p\n", sid, srf->pid, srf->buf);
        }
    }

    dbgf("\nscreen:\n");

    srf = l_mng.head_scr;
    do {
        dbgf("%d ", srf->sid);

        srf = srf->next_scr;
    } while (srf != l_mng.head_scr);

    dbgf("\n\n");
}


//=============================================================================
// 非公開関数


/// 新しい SURFACE を割り当てる。ただし、buf は割り当てない
static SURFACE *srf_alloc(void)
{
    for (int sid = 0; sid < SURFACE_MAX; sid++) {
        SURFACE *srf = &(l_mng.surfaces[sid]);

        if (srf->flags == SRF_FLG_FREE) {
            srf->flags = SRF_FLG_ALLOC;
            srf->pid = get_pid();

            return srf;
        }
    }

    return 0;
}


static void create_mouse_surface(void)
{
    static char cursor[MOUSE_W * MOUSE_H * 2] = {
        "* * . . . . . . . . "
        "* O * . . . . . . . "
        "* O O * . . . . . . "
        "* O O O * . . . . . "
        "* O O O O * . . . . "
        "* O O O O O * . . . "
        "* O O O O O O * . . "
        "* O O O O O O O * . "
        "* O O O O O O O O * "
        "* O O O O O * * * * "
        "* O O * O O * . . . "
        "* O * . * O O * . . "
        "* * . . * O O * . . "
        ". . . . . * O O * . "
        ". . . . . * O O * . "
        ". . . . . . * * * . "
    };

    l_mouse_sid = surface_new(MOUSE_W, MOUSE_H);
    l_mouse_srf = sid2srf(l_mouse_sid);
    unsigned short *buf = l_mouse_srf->buf;
    set_colorkey(l_mouse_sid, COL_RED);

    char *cur = cursor;

    for (int y = 0; y < MOUSE_H; y++) {
        for (int x = 0; x < MOUSE_W * 2; x++) {
            if (*cur == '*') {
                *buf = COL_BLACK;
                buf++;
            } else if (*cur == 'O') {
                *buf = COL_WHITE;
                buf++;
            } else if (*cur == '.') {
                *buf = COL_RED;
                buf++;
            }

            cur++;
        }
    }
}


/// ビットマップコピー
static void blit_src_copy(SURFACE *src, int src_x, int src_y, int w, int h,
        SURFACE *dst, int dst_x, int dst_y)
{
    // src と dst が同じ場合は、重なっている場合も考慮しないといけないので
    // 場合分けが少し複雑になっている。

    unsigned short *src_buf = src->buf + (src_y * src->w) + src_x;
    unsigned short *dst_buf = dst->buf + (dst_y * dst->w) + dst_x;

    if (src->flags & SRF_FLG_HAS_COLORKEY) {
        unsigned short colorkey = src->colorkey;

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
    } else if (src->flags & SRF_FLG_HAS_ALPHA) {
        unsigned char alpha = src->alpha;

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                *dst_buf = RGB(
                        (GET_RED(*src_buf) * (100 - alpha) +
                        GET_RED(*dst_buf) * alpha) / 100,
                        (GET_GREEN(*src_buf) * (100 - alpha) +
                        GET_GREEN(*dst_buf) * alpha) / 100,
                        (GET_BLUE(*src_buf) * (100 - alpha) +
                        GET_BLUE(*dst_buf) * alpha) / 100);

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
                memmove(dst_buf, src_buf, w * h * sizeof (short));
            } else {
                memcpy(dst_buf, src_buf, w * h * sizeof (short));
            }
        } else {  // src と dst の幅が違うなら１行ずつコピー
            if (src == dst) {
                if (src_y >= dst_y) {
                    for (int y = 0; y < h; y++) {
                        memmove(dst_buf, src_buf, w * sizeof (short));

                        src_buf += src->w;
                        dst_buf += dst->w;
                    }
                } else {
                    for (int y = h; y >= 0; y--) {
                        memmove(dst_buf, src_buf, w * sizeof (short));

                        src_buf += src->w;
                        dst_buf += dst->w;
                    }
                }
            } else {
                for (int y = 0; y < h; y++) {
                    memcpy(dst_buf, src_buf, w * sizeof (short));

                    src_buf += src->w;
                    dst_buf += dst->w;
                }
            }
        }
    }
}


/// ビットマップxor。srcとdstの重なりには対応してない
static void blit_src_invert(SURFACE *src, int src_x, int src_y, int w, int h,
        SURFACE *dst, int dst_x, int dst_y)
{
    unsigned short *src_buf = src->buf + (src_y * src->w) + src_x;
    unsigned short *dst_buf = dst->buf + (dst_y * dst->w) + dst_x;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            *dst_buf = *src_buf ^ *dst_buf;

            src_buf++;
            dst_buf++;
        }

        src_buf += (src->w - w);
        dst_buf += (dst->w - w);
    }
}


/// １文字を画面に出力する
static int draw_char(SURFACE *srf, int x, int y, unsigned short color, char ch)
{
    if (x + HANKAKU_W > srf->w || y + HANKAKU_H > srf->h) {
        return x + HANKAKU_W;
    }

    extern char hankaku[4096];
    char *font = hankaku + (((unsigned char) ch) * 16);

    for (int ch_y = 0; ch_y < HANKAKU_H; ch_y++) {
        unsigned short *p = srf->buf + (y + ch_y) * srf->w + x;
        char font_line = font[ch_y];

        for (int ch_x = 0; ch_x < HANKAKU_W; ch_x++) {
            if (font_line & (0x80 >> ch_x)) {
                p[ch_x] = color;
            }
        }
    }

    return x + HANKAKU_W;
}


/// １文字を画面に出力する（背景色を指定）
static int draw_char_bg(SURFACE *srf, int x, int y, unsigned short color,
        unsigned short bg_color, char ch)
{
    if (x + HANKAKU_W > srf->w || y + HANKAKU_H > srf->h) {
        return x + HANKAKU_W;
    }

    extern char hankaku[4096];
    char *font = hankaku + (((unsigned char) ch) * 16);

    for (int ch_y = 0; ch_y < HANKAKU_H; ch_y++) {
        unsigned short *p = srf->buf + (y + ch_y) * srf->w + x;
        char font_line = font[ch_y];

        for (int ch_x = 0; ch_x < HANKAKU_W; ch_x++) {
            if (font_line & (0x80 >> ch_x)) {
                p[ch_x] = color;
            } else {
                p[ch_x] = bg_color;
            }
        }
    }


    return x + HANKAKU_W;
}


static void add_screen_head(SURFACE *srf)
{
    srf->flags |= SRF_FLG_SCREEN;

    if (l_mng.head_scr == 0) {
        srf->prev_scr = srf;
        srf->next_scr = srf;
        l_mng.head_scr = srf;
        return;
    }

    SURFACE *old_head = l_mng.head_scr;
    SURFACE *last = old_head->prev_scr;

    srf->prev_scr = last;
    srf->next_scr = old_head;

    old_head->prev_scr = srf;
    last->next_scr = srf;

    l_mng.head_scr = srf;

    MSG msg;
    msg.message = MSG_SCREEN_SWITCHED;
    msg.u_param = srf->pid;
    msg_q_put(g_root_pid, &msg);
}


static void add_screen_tail(SURFACE *srf)
{
    srf->flags |= SRF_FLG_SCREEN;

    if (l_mng.head_scr == 0) {
        srf->prev_scr = srf;
        srf->next_scr = srf;
        l_mng.head_scr = srf;
        return;
    }

    SURFACE *head = l_mng.head_scr;
    SURFACE *old_last = head->prev_scr;

    srf->prev_scr = old_last;
    srf->next_scr = head;

    head->prev_scr = srf;
    old_last->next_scr = srf;
}


static void remove_from_scr_list(SURFACE *srf)
{
    if ((srf->flags & SRF_FLG_SCREEN) == 0) {
        return;
    }

    srf->flags &= ~SRF_FLG_SCREEN;  // screen flg off

    SURFACE *prev = srf->prev_scr;
    SURFACE *next = srf->next_scr;

    prev->next_scr = next;
    next->prev_scr = prev;

    if (l_mng.head_scr == srf) {
        l_mng.head_scr = next;

        MSG msg;
        msg.message = MSG_SCREEN_SWITCHED;
        msg.u_param = next->pid;
        msg_q_put(g_root_pid, &msg);
    }
}


