/**
 * グラフィック
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_GRAPHIC
#define HEADER_GRAPHIC

#include <stdbool.h>

#define ERROR_SID      (-1)
#define NO_PARENT_SID  (-2)

typedef unsigned short COLOR;

#define RGB(R, G, B) ((COLOR) (((R) & 0xF8) << 8) | (((G) & 0xFC) << 3) | (((B) & 0xF8) >> 3))
#define RGB2(rgb)    ((COLOR) (((rgb) & 0xF80000) >> 8) | (((rgb) & 0xFC00) >> 5) | (((rgb) & 0xF8) >> 3))

#define GET_RED(RGB)   (((RGB) & 0xF800) >> 8)
#define GET_GREEN(RGB) (((RGB) & 0x07E0) >> 3)
#define GET_BLUE(RGB)  (((RGB) & 0x001F) << 3)

#define COL_BLACK    RGB(  0,   0,   0)
#define COL_RED      RGB(255,   0,   0)
#define COL_GREEN    RGB(  0, 255,   0)
#define COL_BLUE     RGB(  0,   0, 255)
#define COL_WHITE    RGB(255, 255, 255)

#define BORDER_WIDTH      (3)
#define TITLE_BAR_HEIGHT  (19)
#define WINDOW_EXT_WIDTH  (BORDER_WIDTH * 2)
#define WINDOW_EXT_HEIGHT (TITLE_BAR_HEIGHT + (BORDER_WIDTH * 2))

#define CLIENT_X  (BORDER_WIDTH)
#define CLIENT_Y  (TITLE_BAR_HEIGHT + BORDER_WIDTH)

#define HANKAKU_W 8   ///< 半角フォントの幅
#define HANKAKU_H 16  ///< 半角フォントの高さ



enum {
    OP_SRC_COPY,
    OP_SRC_INVERT
};


extern int g_vram_sid;
extern int g_dt_sid;

extern const int g_w;
extern const int g_h;


void graphic_init(COLOR *vram);

int  new_window(int x, int y, int cw, int ch, char *title);

int  new_surface(int parent_sid, int w, int h);
int  new_surface_from_buf(int parent_sid, int w, int h, COLOR *buf);
void free_surface(int sid);
void free_surface_task(int pid);
int  get_screen(void);

void set_sprite_pos(int sid, int x, int y);
void move_sprite(int sid, int dx, int dy);

void update_surface(int sid);
void update_rect(int sid, int x, int y, int w, int h);
void update_char(int sid, int x, int y);
void update_text(int sid, int x, int y, int len);

void draw_sprite(int src_sid, int dst_sid, int op);
void blit_surface(int src_sid, int src_x, int src_y, int w, int h,
        int dst_sid, int dst_x, int dst_y, int op);

void fill_surface(int sid, COLOR color);
void fill_rect(int sid, int x, int y, int w, int h, COLOR color);
void draw_text(int sid, int x, int y, COLOR color, const char *text);
void draw_text_bg(int sid, int x, int y, COLOR color,
        COLOR bg_color, const char *text);

void draw_pixel(int sid, unsigned int x, unsigned int y, COLOR color);

void draw_line(int sid, int x0, int y0, int x1, int y1, COLOR color);

void erase_char(int sid, int x, int y, COLOR color, bool update);

void scroll_surface(int sid, int cx, int cy);

void set_colorkey(int sid, COLOR colorkey);
void clear_colorkey(int sid);
void set_alpha(int sid, unsigned char alpha);
void clear_alpha(int sid);

void set_mouse_pos(int x, int y);

int  get_active_win_pid(void);
void switch_window(void);

void graphic_dbg(void);

int hrb_sid2addr(int sid);
int hrb_addr2sid(int addr);

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

#define SRF_FLG_FREE            (0)
#define SRF_FLG_ALLOC           (1)
#define SRF_FLG_HAS_COLORKEY    (2)
#define SRF_FLG_HAS_ALPHA       (4)
#define SRF_FLG_WINDOW          (8)

typedef struct SURFACE {
    int sid;    ///< SURFACE ID
    int flags;
    int pid;    ///< この SURFACE を持っているプロセス ID
    char *name;

    int x;
    int y;
    int w;
    int h;

    COLOR *buf;

    COLOR colorkey;  ///< 透明にする色。SRF_FLG_HAS_COLORKEYで有効。
    unsigned char alpha;

    struct SURFACE *parent;
    struct SURFACE *prev_srf;
    struct SURFACE *next_srf;

    int num_children;
    struct SURFACE *children;
} SURFACE;


typedef struct SURFACE_MNG {
    /// 記憶領域の確保用。
    /// surfaces のインデックスと SID は同じ
    SURFACE surfaces[SURFACE_MAX];
} SURFACE_MNG;


int g_vram_sid;  ///< この SID を使うと更新しなくても画面に直接表示される
int g_dt_sid;

const int g_w = 640;  ///< 横の解像度
const int g_h = 480;  ///< 縦の解像度
const int g_d = 16;   // 色のビット数


static SURFACE_MNG l_mng;

static int l_mouse_sid;
static SURFACE *l_mouse_srf;

static SURFACE *sid2srf(int sid);

static void conv_screen_cord(SURFACE *srf, int *x, int *y);

static SURFACE *srf_alloc(void);

static void create_mouse_surface(void);

static void blit_src_copy(SURFACE *src, int src_x, int src_y, int w, int h,
        SURFACE *dst, int dst_x, int dst_y);
static void blit_src_invert(SURFACE *src, int src_x, int src_y, int w, int h,
        SURFACE *dst, int dst_x, int dst_y);

static int draw_char(SURFACE *srf, int x, int y, COLOR color, char ch);
static int draw_char_bg(SURFACE *srf, int x, int y, COLOR color,
        COLOR bg_color, char ch);

static void add_child(SURFACE *srf);
static void add_child_head(SURFACE *srf);
static void remove_child(SURFACE *srf);

static SURFACE *l_vram_srf;
static SURFACE *l_dt_srf;

//=============================================================================
// 公開関数

void graphic_init(COLOR *vram)
{
    // ---- SURFACE 初期化

    for (int sid = 0; sid < SURFACE_MAX; sid++) {
        SURFACE *srf = &l_mng.surfaces[sid];

        srf->sid = sid;
        srf->flags = SRF_FLG_FREE;
        srf->pid = ERROR_PID;
    }


    // ---- SURFACE の作成

    // -------- VRAM SURFACE の作成

    SURFACE *srf = srf_alloc();
    int sid = srf->sid;

    g_vram_sid = sid;

    srf->w   = g_w;
    srf->h   = g_h;
    srf->buf = vram;

    l_vram_srf = srf;

    // -------- デスクトップ画面の作成

    sid = new_surface(g_vram_sid, g_w, g_h);
    g_dt_sid = sid;
    srf = sid2srf(sid);
    srf->pid = g_root_pid;

    l_dt_srf = srf;

    // -------- マウス SURFACE の作成

    create_mouse_surface();

}

static void draw_window(int sid);

int new_window(int x, int y, int cw, int ch, char *title)
{
    int w = cw + WINDOW_EXT_WIDTH;
    int h = ch + WINDOW_EXT_HEIGHT;

    int parent_sid = g_dt_sid;

    int sid = new_surface(parent_sid, w, h);

    if (sid == ERROR_SID) {
        return sid;
    }

    SURFACE *srf = sid2srf(sid);

    srf->name = title;

    draw_window(sid);
    set_sprite_pos(sid, x, y);
    update_surface(sid);

    srf->flags |= SRF_FLG_WINDOW;

    return sid;
}


static void draw_window(int sid)
{
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };

    SURFACE *srf = sid2srf(sid);

    int w = srf->w;
    int h = srf->h;
    int cw = w - WINDOW_EXT_WIDTH;
    int ch = h - WINDOW_EXT_HEIGHT;

    /* top border */
    fill_rect(sid,   0,   0,   w,   1, RGB2(0xC6C6C6));
    fill_rect(sid,   1,   1, w-2,   1, RGB2(0xFFFFFF));

    /* left border */
    fill_rect(sid,   0,   0,   1,   h, RGB2(0xC6C6C6));
    fill_rect(sid,   1,   1,   1, h-2, RGB2(0xFFFFFF));

    /* right border */
    fill_rect(sid, w-2,   1,   1, h-2, RGB2(0x848484));
    fill_rect(sid, w-1,   0,   1,   h, RGB2(0x000000));

    /* bottom border */
    fill_rect(sid,   1, h-2, w-2,   1, RGB2(0x848484));
    fill_rect(sid,   0, h-1,   w,   1, RGB2(0x000000));

    /* title bar */
    fill_rect(sid,   2,   2, w-4, h-4, RGB2(0xC6C6C6));
    fill_rect(sid,   3,   3, w-6, TITLE_BAR_HEIGHT - 1, RGB2(0x000084));

    /* client area */
    fill_rect(sid, CLIENT_X, CLIENT_Y, cw, ch, RGB2(0xFFFFFF));

    draw_text(sid, 6, 4, RGB2(0xFFFFFF), srf->name);

    /* close button */
    COLOR color_at     = RGB2(0x000000);
    COLOR color_dollar = RGB2(0x848484);
    COLOR color_q      = RGB2(0xFFFFFF);
    COLOR color;

    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 16; x++) {
            char c = closebtn[y][x];

            switch (c) {
            case '@':
                color = color_at;
                break;

            case '$':
                color = color_dollar;
                break;

            case 'Q':
                color = color_q;
                break;

            default:
                color = color_q;
                break;
            }

            draw_pixel(sid, w - 21 + x, y + 5, color);
        }
    }
}


/// 新しい SURFACE を作成する
int new_surface(int parent_sid, int w, int h)
{
    int size = w * h * sizeof (COLOR);
    COLOR *buf = mem_alloc(size);

    return new_surface_from_buf(parent_sid, w, h, buf);
}


/// @attention buf は mem_alloc で取得したメモリを使用すること
int new_surface_from_buf(int parent_sid, int w, int h, COLOR *buf)
{
    SURFACE *parent = 0;

    if (parent_sid != NO_PARENT_SID) {
        parent = sid2srf(parent_sid);

        if (parent == 0) {
            DBGF("invalid parent sid");
            return ERROR_SID;
        }
    }

    SURFACE *srf = srf_alloc();

    if (srf == 0) {
        DBGF("could't create surface");
        return ERROR_SID;
    }
 
    srf->pid = get_pid();
    srf->x    = 0;
    srf->y    = 0;
    srf->w    = w;
    srf->h    = h;
    srf->buf = buf;
    srf->parent = parent;

    if (parent_sid == g_dt_sid) {
        add_child_head(srf);
    } else {
        add_child(srf);
    }

    return srf->sid;
}


void free_surface(int sid)
{
    if (sid == g_vram_sid || sid == g_dt_sid) {
        return;
    }

    SURFACE *srf = sid2srf(sid);

    if (srf == 0 || (srf->flags & SRF_FLG_ALLOC) == 0) {
        return;
    }

    if (srf->pid != get_pid()) {
        return;
    }

    SURFACE *parent = srf->parent;

    remove_child(srf);

    mem_free(srf->buf);
    srf->flags = SRF_FLG_FREE;
    srf->pid = ERROR_PID;

    if (parent != 0) {
        update_surface(parent->sid);
    }
}


void free_surface_task(int pid)
{
    for (int sid = 0; sid < SURFACE_MAX; sid++) {
        SURFACE *srf = &(l_mng.surfaces[sid]);

        if (srf->pid != pid || (srf->flags & SRF_FLG_ALLOC) == 0) {
            continue;
        }

        remove_child(srf);

        mem_free(srf->buf);
        srf->flags = SRF_FLG_FREE;
        srf->pid = ERROR_PID;
    }
}


int get_screen(void)
{
    return l_vram_srf->children->sid;
}


void set_sprite_pos(int sid, int x, int y)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    srf->x = x;
    srf->y = y;
}


void move_sprite(int sid, int dx, int dy)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    SURFACE *parent = srf->parent;

    if (parent == 0 || parent == l_vram_srf) {
        return;
    }

    srf->x = MAXMIN(0, srf->x + dx, g_w);
    srf->y = MAXMIN(0, srf->y + dy, g_h);
}

static void update_rect0(int sid, int x, int y, int w, int h, bool window);

void update_surface(int sid)
{
    update_rect0(sid, 0, 0, 0, 0, true);
}

void update_rect(int sid, int x, int y, int w, int h)
{
    update_rect0(sid, x, y, w, h, false);
}

static void update_rect_sub(SURFACE *srf, int p_x, int p_y, int x, int y, int w, int h);

static void update_rect0(int sid, int x, int y, int w, int h, bool window)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (w == 0 && h == 0) {
        w = srf->w;
        h = srf->h;
    }

    int p_x = x;
    int p_y = y;

    conv_screen_cord(srf, &p_x, &p_y);

    if (srf->flags & SRF_FLG_WINDOW) {
        if (window) {
            p_x -= CLIENT_X;
            p_y -= CLIENT_Y;
        } else {
            x += CLIENT_X;
            y += CLIENT_Y;
        }
    }

    update_rect_sub(srf, p_x, p_y, x, y, w, h);

    int mx = l_mouse_srf->x;
    int my = l_mouse_srf->y;

    if (x <= mx && mx <= x + w && y <= my && my <= y + h) {
        // マウスカーソルを更新する
        blit_surface(l_mouse_sid, 0, 0, MOUSE_W, MOUSE_H,
                g_vram_sid, mx, my, OP_SRC_COPY);
    }
}


static void update_rect_sub(SURFACE *srf, int p_x, int p_y, int x, int y, int w, int h)
{
    blit_surface(srf->sid, x, y, w, h, g_vram_sid, p_x, p_y, OP_SRC_COPY);

    if (srf->num_children == 0)
        return;

    SURFACE *p = srf->children;
    do {
        update_rect_sub(p, p_x, p_y, x - p->x, y - p->y, w, h);

        p = p->next_srf;
    } while (p != srf->children);
}


void update_text(int sid, int x, int y, int len)
{
    update_rect(sid, x, y, len * HANKAKU_W, HANKAKU_H);
}


void update_char(int sid, int x, int y)
{
    update_rect(sid, x, y, HANKAKU_W, HANKAKU_H);
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

    int src_max_w = src->w - src_x;
    int src_max_h = src->h - src_y;
    int dst_max_w = dst->w - dst_x;
    int dst_max_h = dst->h - dst_y;

    if (src->flags & SRF_FLG_WINDOW) {
        // src_x, src_yはクライアント座標なので余分に引き過ぎた分を戻す
        src_max_w += CLIENT_X;
        src_max_h += CLIENT_Y;

        src_max_w -= WINDOW_EXT_WIDTH;
        src_max_h -= WINDOW_EXT_HEIGHT;
    }

    if (dst->flags & SRF_FLG_WINDOW) {
        // dst_x, dst_yはクライアント座標なので余分に引き過ぎた分を戻す
        dst_max_w += CLIENT_X;
        dst_max_h += CLIENT_Y;

        dst_max_w -= WINDOW_EXT_WIDTH;
        dst_max_h -= WINDOW_EXT_HEIGHT;
    }

    src_x = MAXMIN(0, src_x, src->w);
    src_y = MAXMIN(0, src_y, src->h);
    dst_x = MAXMIN(0, dst_x, dst->w);
    dst_y = MAXMIN(0, dst_y, dst->h);

    if (src == dst && src_x == dst_x && src_y == dst_y) {
        return;
    }

    w = MIN(w, MIN(src_max_w, dst_max_w));
    h = MIN(h, MIN(src_max_h, dst_max_h));


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
void fill_surface(int sid, COLOR color)
{
    fill_rect(sid, 0, 0, 0, 0, color);
}


/// 矩形を塗りつぶす
void fill_rect(int sid, int x, int y, int w, int h, COLOR color)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (w == 0 && h == 0) {
        w = srf->w;
        h = srf->h;
    }

    int max_w = srf->w - x;
    int max_h = srf->h - y;

    if (srf->flags & SRF_FLG_WINDOW) {
        x += CLIENT_X;
        y += CLIENT_Y;
        max_w -= WINDOW_EXT_WIDTH;
        max_h -= WINDOW_EXT_HEIGHT;
    }

    x = MAXMIN(0, x, srf->w);
    y = MAXMIN(0, y, srf->h);
    w = MAXMIN(0, w, max_w);
    h = MAXMIN(0, h, max_h);

    COLOR *buf = srf->buf + (y * srf->w) + x;
    COLOR *buf1 = buf;

    for (int px = 0; px < w; px++) {
        buf1[px] = color;
    }
    buf += srf->w;

    for (int line = 1; line < h; line++) {
        memcpy(buf, buf1, w * sizeof (COLOR));
        buf += srf->w;
    }
}


/// 文字列を画面に出力する
void draw_text(int sid, int x, int y, COLOR color, const char *s)
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
void draw_text_bg(int sid, int x, int y, COLOR color,
        COLOR bg_color, const char *s)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    for ( ; *s; s++) {
        x = draw_char_bg(srf, x, y, color, bg_color, *s);
    }
    update_surface(sid);
}


void draw_pixel(int sid, unsigned int x, unsigned int y, COLOR color)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    int max_w = srf->w;
    int max_h = srf->h;

    if (srf->flags & SRF_FLG_WINDOW) {
        x += CLIENT_X;
        y += CLIENT_Y;
        max_w -= WINDOW_EXT_WIDTH;
        max_h -= WINDOW_EXT_HEIGHT;
    }

    if (x < srf->w && y <= srf->h) {
        srf->buf[srf->w * y + x] = color;
    }
}


/**
 * 線を引く。はりぼてOSから持ってきた
 */
void draw_line(int sid, int x0, int y0, int x1, int y1, COLOR color)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (srf->flags & SRF_FLG_WINDOW) {
        x0 += CLIENT_X;
        y0 += CLIENT_Y;
        x1 += CLIENT_X;
        y1 += CLIENT_Y;
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    int x = x0 << 10;
    int y = y0 << 10;

    if (dx < 0)
        dx = -dx;

    if (dy < 0)
        dy = -dy;

    int len;

    if (dx >= dy) {
        len = dx + 1;

        if (x0 > x1) {
            dx = -1024;
        } else {
            dx =  1024;
        }

        if (y0 <= y1) {
            dy = ((y1 - y0 + 1) << 10) / len;
        } else {
            dy = ((y1 - y0 - 1) << 10) / len;
        }
    } else {
        len = dy + 1;

        if (y0 > y1) {
            dy = -1024;
        } else {
            dy =  1024;
        }

        if (x0 <= x1) {
            dx = ((x1 - x0 + 1) << 10) / len;
        } else {
            dx = ((x1 - x0 - 1) << 10) / len;
        }
    }

    for (int i = 0; i < len; i++) {
        srf->buf[(y >> 10) * srf->w + (x >> 10)] = color;
        x += dx;
        y += dy;
    }
}



void erase_char(int sid, int x, int y, COLOR color, bool update)
{
    fill_rect(sid, x, y, HANKAKU_W, HANKAKU_H, color);

    if (update) {
        update_char(sid, x, y);
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

    if (srf->flags & SRF_FLG_WINDOW) {
        src_x += CLIENT_X;
        src_y += CLIENT_Y;
        dst_x += CLIENT_X;
        dst_y += CLIENT_Y;
    }

    blit_surface(sid, src_x, src_y, srf->w, srf->h, sid, dst_x, dst_y,
            OP_SRC_COPY);
}


void set_colorkey(int sid, COLOR colorkey)
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

    blit_surface(l_vram_srf->children->sid, mx, my, MOUSE_W, MOUSE_H, g_vram_sid,
            mx, my, OP_SRC_COPY);

    l_mouse_srf->x = x;
    l_mouse_srf->y = y;

    blit_surface(l_mouse_sid, 0, 0, MOUSE_W, MOUSE_H, g_vram_sid,
            x, y, OP_SRC_COPY);
}


int get_active_win_pid(void)
{
    if (l_dt_srf == 0 || l_dt_srf->num_children == 0)
        return ERROR_PID;

    return l_dt_srf->children->pid;
}


void switch_window(void)
{
    // l_dt_srf->children の先頭が現在アクティブなウィンドウを表している。

    if (l_dt_srf == 0 || l_dt_srf->num_children == 0)
        return;

    SURFACE *old_head = l_dt_srf->children;
    remove_child(old_head);
    add_child(old_head);

    update_surface(l_vram_srf->children->sid);
}


void graphic_dbg(void)
{
    dbgf("\n");
    DBGF("DEBUG GRAPHIC");

    SURFACE *srf;
    dbgf("%d %d %d %d\n", CLIENT_X, CLIENT_Y, WINDOW_EXT_WIDTH, WINDOW_EXT_HEIGHT);

    dbgf("surfaces:\n");
    for (int sid = 0; sid < SURFACE_MAX; sid++) {
        srf = &l_mng.surfaces[sid];

        if (srf->flags & SRF_FLG_ALLOC) {
            dbgf("%d: pid = %d, buf = %p, (%d, %d, %d, %d)\n", sid, srf->pid, srf->buf,
                    srf->x, srf->y, srf->w, srf->h);

            if (srf->num_children == 0)
                continue;

            SURFACE *p = srf->children;
            dbgf("    children: ");
            do {
                dbgf("%d ", p->sid);
            } while (p != srf->children);
            dbgf("\n");
        }
    }

    dbgf("\nscreen:\n");

    srf = l_vram_srf->children;
    do {
        dbgf("%d ", srf->sid);

        srf = srf->next_srf;
    } while (srf != l_vram_srf->children);

    dbgf("\n\n");
}


/**
 * はりぼて互換のため
 */
int hrb_sid2addr(int sid)
{
    return (int) sid2srf(sid);
}


/**
 * はりぼて互換のため
 */
int hrb_addr2sid(int addr)
{
    SURFACE *srf = (SURFACE *) addr;

    return srf->sid;
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

            srf->parent = 0;
            srf->num_children = 0;
            srf->children = 0;
            srf->x = srf->y = 0;
            srf->prev_srf = srf;
            srf->next_srf = srf;

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

    l_mouse_sid = new_surface(NO_PARENT_SID, MOUSE_W, MOUSE_H);
    l_mouse_srf = sid2srf(l_mouse_sid);
    COLOR *buf = l_mouse_srf->buf;
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

    COLOR *src_buf = src->buf + (src_y * src->w) + src_x;
    COLOR *dst_buf = dst->buf + (dst_y * dst->w) + dst_x;

    if (src->flags & SRF_FLG_HAS_COLORKEY) {
        COLOR colorkey = src->colorkey;

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
                memmove(dst_buf, src_buf, w * h * sizeof (COLOR));
            } else {
                memcpy(dst_buf, src_buf, w * h * sizeof (COLOR));
            }
        } else {  // src と dst の幅が違うなら１行ずつコピー
            if (src == dst) {
                if (src_y >= dst_y) {
                    for (int y = 0; y < h; y++) {
                        memmove(dst_buf, src_buf, w * sizeof (COLOR));

                        src_buf += src->w;
                        dst_buf += dst->w;
                    }
                } else {
                    for (int y = h; y >= 0; y--) {
                        memmove(dst_buf, src_buf, w * sizeof (COLOR));

                        src_buf += src->w;
                        dst_buf += dst->w;
                    }
                }
            } else {
                for (int y = 0; y < h; y++) {
                    memcpy(dst_buf, src_buf, w * sizeof (COLOR));

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
    COLOR *src_buf = src->buf + (src_y * src->w) + src_x;
    COLOR *dst_buf = dst->buf + (dst_y * dst->w) + dst_x;

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
static int draw_char(SURFACE *srf, int x, int y, COLOR color, char ch)
{
    int old_x = x;

    if (srf->flags & SRF_FLG_WINDOW) {
        x += CLIENT_X;
        y += CLIENT_Y;
    }

    if (x + HANKAKU_W > srf->w || y + HANKAKU_H > srf->h) {
        return old_x + HANKAKU_W;
    }

    extern char hankaku[4096];
    char *font = hankaku + (((unsigned char) ch) * 16);

    for (int ch_y = 0; ch_y < HANKAKU_H; ch_y++) {
        COLOR *p = srf->buf + (y + ch_y) * srf->w + x;
        char font_line = font[ch_y];

        for (int ch_x = 0; ch_x < HANKAKU_W; ch_x++) {
            if (font_line & (0x80 >> ch_x)) {
                p[ch_x] = color;
            }
        }
    }

    return old_x + HANKAKU_W;
}


/// １文字を画面に出力する（背景色を指定）
static int draw_char_bg(SURFACE *srf, int x, int y, COLOR color,
        COLOR bg_color, char ch)
{
    int old_x = x;

    if (srf->flags & SRF_FLG_WINDOW) {
        x += CLIENT_X;
        y += CLIENT_Y;
    }

    if (x + HANKAKU_W > srf->w || y + HANKAKU_H > srf->h) {
        return old_x + HANKAKU_W;
    }

    extern char hankaku[4096];
    char *font = hankaku + (((unsigned char) ch) * 16);

    for (int ch_y = 0; ch_y < HANKAKU_H; ch_y++) {
        COLOR *p = srf->buf + (y + ch_y) * srf->w + x;
        char font_line = font[ch_y];

        for (int ch_x = 0; ch_x < HANKAKU_W; ch_x++) {
            if (font_line & (0x80 >> ch_x)) {
                p[ch_x] = color;
            } else {
                p[ch_x] = bg_color;
            }
        }
    }

    return old_x + HANKAKU_W;
}


static void add_child(SURFACE *srf)
{
    SURFACE *parent = srf->parent;

    if (parent == 0) {
        return;
    }

    if (parent->num_children == 0) {
        parent->children = srf;
    } else {
        SURFACE *head = parent->children;
        SURFACE *old_last = head->prev_srf;

        srf->prev_srf = old_last;
        srf->next_srf = head;

        head->prev_srf = srf;
        old_last->next_srf = srf;
    }

    parent->num_children++;
}


static void add_child_head(SURFACE *srf)
{
    SURFACE *parent = srf->parent;

    if (parent == 0) {
        return;
    }

    if (parent->num_children == 0) {
        parent->children = srf;
    } else {
        SURFACE *old_head = parent->children;
        SURFACE *last = old_head->prev_srf;

        srf->prev_srf = last;
        srf->next_srf = old_head;

        old_head->prev_srf = srf;
        last->next_srf = srf;

        parent->children = srf;
    }

    parent->num_children++;

    if (parent == l_dt_srf) {
        MSG msg;
        msg.message = MSG_WINDOW_SWITCHED;
        msg.u_param = srf->pid;
        msg_q_put(g_root_pid, &msg);
    }
}


static void remove_child(SURFACE *srf)
{
    SURFACE *parent = srf->parent;

    if (parent == 0) {
        return;
    }

    if (parent->num_children == 0) {
        DBGF("*** FOUND BUG! ***");
        return;
    }

    if (parent->num_children == 1) {
        if (parent->children != srf) {
            DBGF("couldn't remove child");
            return;
        }

        if (parent == l_dt_srf) {
            MSG msg;
            msg.message = MSG_WINDOW_SWITCHED;
            msg.u_param = ERROR_PID;
            msg_q_put(g_root_pid, &msg);
        }

        parent->children = 0;
        parent->num_children--;
        return;
    }

    SURFACE *s = parent->children;
    do {
        if (s == srf) {
            SURFACE *prev = srf->prev_srf;
            SURFACE *next = srf->next_srf;

            prev->next_srf = next;
            next->prev_srf = prev;

            if (s == parent->children) {
                parent->children = next;

                if (parent == l_dt_srf) {
                    MSG msg;
                    msg.message = MSG_WINDOW_SWITCHED;
                    msg.u_param = parent->children->pid;
                    msg_q_put(g_root_pid, &msg);
                }
            }

            return;
        }

        s = s->next_srf;
    } while (s != parent->children);
}


static SURFACE *sid2srf(int sid)
{
    if (sid < 0 || SURFACE_MAX <= sid) {
        return 0;
    }

    return &l_mng.surfaces[sid];
}


static void conv_screen_cord(SURFACE *srf, int *x, int *y)
{
    while (srf) {
        *x += srf->x;
        *y += srf->y;

        if (srf->flags & SRF_FLG_WINDOW) {
            *x += CLIENT_X;
            *y += CLIENT_Y;
        }

        srf = srf->parent;
    }
}
