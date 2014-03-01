/**
 * グラフィック
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_GRAPHIC
#define HEADER_GRAPHIC

#include <stdbool.h>
#include "color.h"
#include "gbuffer.h"

#define ERROR_SID      (-1)
#define NO_PARENT_SID  (-2)

#define BORDER_WIDTH      (4)
#define TITLE_BAR_HEIGHT  (20)
#define WINDOW_EXT_WIDTH  (BORDER_WIDTH * 2)
#define WINDOW_EXT_HEIGHT (TITLE_BAR_HEIGHT + (BORDER_WIDTH * 2))

#define CLIENT_X  (BORDER_WIDTH)
#define CLIENT_Y  (TITLE_BAR_HEIGHT + BORDER_WIDTH)

#define HANKAKU_W (8)   // 半角フォントの幅
#define HANKAKU_H (16)  // 半角フォントの高さ


enum {
    OP_SRC_COPY,
    OP_SRC_INVERT
};


extern int g_vram_sid;
extern int g_dt_sid;

extern const int g_w;
extern const int g_h;


void graphic_init(void *vram);

int  new_surface(int parent_sid, int w, int h);
int  new_surface_from_buf(int parent_sid, int w, int h, void *buf, int color_width);

int  new_window(int x, int y, int w, int h, char *title);
int  new_window_from_buf(int x, int y, int w, int h, char *title,
        void *buf, int color_width);

void free_surface(int sid);
void free_surface_task(int pid);

int  get_screen(void);

void set_surface_pos(int sid, int x, int y);
void move_surface(int sid, int x, int y);

void update_surface(int sid);
void update_window(int pid);
void update_from_buf(void);
void update_rect(int sid, int x, int y, int w, int h);
void update_char(int sid, int x, int y);
void update_text(int sid, int x, int y, int len);

void draw_surface(int src_sid, int dst_sid, int x, int y, int op);
void draw_surface2(int src_sid, int dst_sid, int op);

void fill_surface(int sid, COLOR32 color);
void fill_rect(int sid, int x, int y, int w, int h, COLOR32 color);
void draw_text(int sid, int x, int y, COLOR32 color, const char *text);
void draw_text_bg(int sid, int x, int y, COLOR32 color,
        COLOR32 bg_color, const char *text);

void draw_pixel(int sid, unsigned int x, unsigned int y, COLOR32 color);

void draw_line(int sid, int x0, int y0, int x1, int y1, COLOR32 color);

void erase_char(int sid, int x, int y, COLOR32 color, bool update);

void scroll_surface(int sid, int cx, int cy);

void set_colorkey(int sid, COLOR32 colorkey);
void clear_colorkey(int sid);
void set_alpha(int sid, unsigned char alpha);
void clear_alpha(int sid);

void set_mouse_pos(int x, int y);

void graphic_left_down(int x, int y);
void graphic_left_up(int x, int y);
void graphic_left_drag(int x, int y);

void switch_window(void);

void graphic_dbg(void);

int hrb_sid2addr(int sid);
int hrb_addr2sid(int addr);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "debug.h"
#include "graphic.h"
#include "memory.h"
#include "mouse.h"
#include "msg_q.h"
#include "stacktrace.h"
#include "str.h"
#include "task.h"


#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAXMIN(A, B, C) MAX(A, MIN(B, C))
#define ABS(X) ((X) < 0 ? -(X) : X)


#define SURFACE_MAX (1024)

#define CLOSE_BTN_W  (16)
#define CLOSE_BTN_H  (14)

#define MOUSE_W      (10)
#define MOUSE_H      (16)

#define SRF_FLG_FREE            (0)
#define SRF_FLG_ALLOC           (1 << 0)
#define SRF_FLG_WINDOW          (1 << 1)
#define SRF_FLG_WIN_ACTIVE      (1 << 2)

typedef struct SURFACE {
    int sid;    ///< SURFACE ID
    unsigned int flags;
    int pid;    ///< この SURFACE を持っているプロセス ID
    char *name;

    int x;
    int y;

    GBUFFER g;

    struct SURFACE *parent;
    struct SURFACE *prev_srf;
    struct SURFACE *next_srf;

    int num_children;
    struct SURFACE *children;
} SURFACE;


typedef struct SURFACE_MNG {
    // 記憶領域の確保用。
    // surfaces のインデックスと SID は同じ
    SURFACE surfaces[SURFACE_MAX];
} SURFACE_MNG;


int g_vram_sid;  // この SID を使うと更新しなくても画面に直接表示される
int g_dt_sid;

const int g_w = (640);  // 横の解像度
const int g_h = (480);  // 縦の解像度
const int g_d = (16);   // 色のビット数


static SURFACE_MNG l_mng;

static int l_close_button_sid;
static SURFACE *l_close_button_srf;

static int l_mouse_sid;
static SURFACE *l_mouse_srf;

static SURFACE *sid2srf(int sid);

static void conv_screen_cord(SURFACE *srf, int *x, int *y, bool client_cord);

static SURFACE *srf_alloc(void);

static void create_close_button_surface(void);
static void create_mouse_surface(void);

static int draw_char(SURFACE *srf, int x, int y, COLOR32 color, char ch);
static int draw_char_bg(SURFACE *srf, int x, int y, COLOR32 color,
        COLOR32 bg_color, char ch);

static void add_child(SURFACE *srf);
static void add_child_head(SURFACE *srf);
static void remove_child(SURFACE *srf);

static SURFACE *get_active_win(void);

static SURFACE *l_vram_srf;
static SURFACE *l_dt_srf;
static int l_buf_sid;
static SURFACE *l_buf_srf;

//=============================================================================
// 公開関数

void graphic_init(void *vram)
{
    color_init();

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

    srf->g.w   = g_w;
    srf->g.h   = g_h;
    srf->g.color_width = 16;
    srf->g.buf = vram;
    srf->g.m   = g_gbuf_method16;

    l_vram_srf = srf;

    // -------- デスクトップ画面の作成

    sid = new_surface(NO_PARENT_SID, g_w, g_h);
    g_dt_sid = sid;
    srf = sid2srf(sid);
    srf->pid = g_root_pid;

    l_dt_srf = srf;

    // -------- バッファの作成

    sid = new_surface(NO_PARENT_SID, g_w, g_h);
    l_buf_sid = sid;
    srf = sid2srf(sid);
    srf->pid = g_root_pid;

    l_buf_srf = srf;

    // -------- 閉じるボタン SURFACE の作成

    create_close_button_surface();

    // -------- マウス SURFACE の作成

    create_mouse_surface();

}

static void draw_window(int sid);

int new_window(int x, int y, int w, int h, char *title)
{
    return new_window_from_buf(x, y, w, h, title, 0, 16);
}


int new_window_from_buf(int x, int y, int cw, int ch, char *title,
        void *buf, int color_width)
{
    int w = cw + WINDOW_EXT_WIDTH;
    int h = ch + WINDOW_EXT_HEIGHT;

    int sid;

    if (buf == 0) {
        sid = new_surface(NO_PARENT_SID, w, h);
    } else {
        sid = new_surface_from_buf(NO_PARENT_SID, w, h, buf, color_width);
    }

    if (sid == ERROR_SID) {
        return sid;
    }

    SURFACE *srf = sid2srf(sid);

    srf->name = mem_alloc_str(title);
    srf->flags |= SRF_FLG_WINDOW;
    set_surface_pos(sid, x, y);

    draw_window(sid);

    // 描画したあとに更新するために、ここで親に追加
    srf->parent = l_dt_srf;
    add_child(srf);

    update_surface(srf->sid);

    return sid;
}

static void draw_win_titlebar(SURFACE *srf);

static void draw_window(int sid)
{
    SURFACE *srf = sid2srf(sid);

    int w = srf->g.w;
    int h = srf->g.h;
    int cw = w - WINDOW_EXT_WIDTH;
    int ch = h - WINDOW_EXT_HEIGHT;

    unsigned int flags = srf->flags;
    srf->flags &= ~SRF_FLG_WINDOW;

    /* top border */
    fill_rect(sid,   0,   0,   w,   1, 0x969696);
    fill_rect(sid,   1,   1, w-2,   1, 0xC6C6C6);
    fill_rect(sid,   2,   2, w-4,   1, 0xFFFFFF);

    /* left border */
    fill_rect(sid,   0,   0,   1,   h, 0x969696);
    fill_rect(sid,   1,   1,   1, h-2, 0xC6C6C6);
    fill_rect(sid,   2,   2,   1, h-4, 0xFFFFFF);

    /* right border */
    fill_rect(sid, w-3,   2,   1, h-4, 0x848484);
    fill_rect(sid, w-2,   1,   1, h-2, 0x444444);
    fill_rect(sid, w-1,   0,   1,   h, 0x000000);

    /* bottom border */
    fill_rect(sid,   2, h-3, w-4,   1, 0x848484);
    fill_rect(sid,   1, h-2, w-2,   1, 0x444444);
    fill_rect(sid,   0, h-1,   w,   1, 0x000000);

    /* title bar */
    draw_win_titlebar(srf);

    /* client area */
    fill_rect(sid, CLIENT_X, CLIENT_Y, cw, ch, 0xFFFFFF);

    srf->flags = flags;
}

static void draw_win_titlebar(SURFACE *srf)
{
    unsigned int flags = srf->flags;
    srf->flags &= ~SRF_FLG_WINDOW;

    COLOR32 tbc;

    if (srf->flags & SRF_FLG_WIN_ACTIVE) {
        tbc = 0x000084;
    } else {
        tbc = 0x848484;
    }

    fill_rect(srf->sid, 3, 3, srf->g.w-6, TITLE_BAR_HEIGHT, tbc);

    draw_text(srf->sid, 7, 6, 0xFFFFFF, srf->name);

    draw_surface(l_close_button_sid, srf->sid, srf->g.w  - 21, 7, OP_SRC_COPY);

    srf->flags = flags;
}


/// 新しい SURFACE を作成する
int new_surface(int parent_sid, int w, int h)
{
    int size = w * h * sizeof (COLOR16);
    void *buf = mem_alloc(size);

    return new_surface_from_buf(parent_sid, w, h, buf, 16);
}


/// @attention buf は mem_alloc で取得したメモリを使用すること
int new_surface_from_buf(int parent_sid, int w, int h, void *buf, int color_width)
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
 
    srf->pid    = get_pid();
    srf->x      = 0;
    srf->y      = 0;

    srf->g.w    = w;
    srf->g.h    = h;
    srf->g.color_width = color_width;
    srf->g.buf  = buf;

    if (color_width == 8) {
        srf->g.m = g_gbuf_method8;
    } else {
        srf->g.m = g_gbuf_method16;
    }

    srf->parent = parent;

    add_child(srf);

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

    remove_child(srf);

    if (srf->name)
        mem_free(srf->name);

    mem_free(srf->g.buf);
    srf->flags = SRF_FLG_FREE;
    srf->pid = ERROR_PID;

    update_surface(g_dt_sid);
}


void free_surface_task(int pid)
{
    for (int sid = 0; sid < SURFACE_MAX; sid++) {
        SURFACE *srf = &(l_mng.surfaces[sid]);

        if (srf->pid != pid || (srf->flags & SRF_FLG_ALLOC) == 0) {
            continue;
        }

        remove_child(srf);

        if (srf->name)
            mem_free(srf->name);

        mem_free(srf->g.buf);
        srf->flags = SRF_FLG_FREE;
        srf->pid = ERROR_PID;
    }

    update_surface(g_dt_sid);
}


void set_surface_pos(int sid, int x, int y)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    srf->x = x;
    srf->y = y;
}


void move_surface(int sid, int x, int y)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    SURFACE *parent = srf->parent;

    if (parent == 0 || parent == l_vram_srf) {
        return;
    }

    int old_x = 0;
    int old_y = 0;
    int w = srf->g.w;
    int h = srf->g.h;

    conv_screen_cord(srf, &old_x, &old_y, false);

    srf->x = MIN(x, g_w);
    srf->y = MIN(y, g_h);

    update_rect(parent->sid, old_x, old_y, w, h);
    update_surface(srf->sid);
}


static void update_rect0(int sid, int x, int y, int w, int h, bool client_cord);

void update_surface(int sid)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    cli();

    if (srf->flags & SRF_FLG_WINDOW) {
        draw_win_titlebar(srf);
    }

    update_rect0(sid, 0, 0, 0, 0, false);

    sti();
}


void update_window(int pid)
{
    if (l_dt_srf->num_children == 0)
        return;

    SURFACE *srf = l_dt_srf->children;

    do {
        if (srf->pid == pid) {
            update_surface(srf->sid);
        }
    } while ((srf = srf->next_srf) != l_dt_srf->children);
}


void update_from_buf(void)
{
    GBUFFER *g = &l_buf_srf->g;
    g->m->blit(g, 0, 0, g_w, g_h, &l_vram_srf->g, 0, 0, OP_SRC_COPY);
}


void update_rect(int sid, int x, int y, int w, int h)
{
    cli();

    update_rect0(sid, x, y, w, h, true);

    sti();
}

static void update_rect_sub(SURFACE *srf, int x, int y, int w, int h);

static void update_rect0(int sid, int x, int y, int w, int h, bool client_cord)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (w == 0 && h == 0) {
        w = srf->g.w;
        h = srf->g.h;
    }

    int screen_x = x;
    int screen_y = y;

    conv_screen_cord(srf, &screen_x, &screen_y, client_cord);

    w = MAXMIN(0, w, g_w - screen_x);
    h = MAXMIN(0, h, g_h - screen_y);

    if (w == 0 && h == 0)
        return;

    update_rect_sub(srf, screen_x, screen_y, w, h);

    GBUFFER *g = &l_buf_srf->g;
    g->m->blit(g, screen_x, screen_y, w, h,
            &l_vram_srf->g, screen_x, screen_y, OP_SRC_COPY);

    int mx = l_mouse_srf->x;
    int my = l_mouse_srf->y;

    if (x <= mx && mx <= x + w && y <= my && my <= y + h) {
        // マウスカーソルを更新する
        g = &l_mouse_srf->g;
        g->m->blit(g, 0, 0, MOUSE_W, MOUSE_H,
                &l_vram_srf->g, mx, my, OP_SRC_COPY);
    }
}


static void update_rect_sub(SURFACE *srf, int x, int y, int w, int h)
{
    int l_x = 0, l_y = 0;
    conv_screen_cord(srf, &l_x, &l_y, false);

    /* srf領域と更新領域の重なりを計算 */
    int u_x0 = MAX(l_x, x);
    int u_y0 = MAX(l_y, y);
    int u_x1 = MIN(l_x + srf->g.w, x + w);
    int u_y1 = MIN(l_y + srf->g.h, y + h);
    int u_w = u_x1 - u_x0;
    int u_h = u_y1 - u_y0;

    if (u_w > 0 && u_h > 0) {

        /* 自分を更新 */
        GBUFFER *g = &srf->g;
        g->m->blit(g, ABS(l_x - u_x0), ABS(l_y - u_y0), u_w, u_h,
                &l_buf_srf->g, u_x0, u_y0, OP_SRC_COPY);

        /* 子どもたちを更新 */

        if (srf->num_children != 0) {
            SURFACE *p = srf->children;
            do {
                update_rect_sub(p, x, y, w, h);
            } while ((p = p->next_srf) != srf->children);
        }
    }


    /* 右の兄弟を更新 */

    if (srf->parent == 0)
        return;

    SURFACE *p = srf;
    while ((p = p->next_srf) != srf->parent->children) {
        update_rect_sub(p, x, y, w, h);
    }
}


void update_text(int sid, int x, int y, int len)
{
    update_rect(sid, x, y, len * HANKAKU_W, HANKAKU_H);
}


void update_char(int sid, int x, int y)
{
    update_rect(sid, x, y, HANKAKU_W, HANKAKU_H);
}


void draw_surface(int src_sid, int dst_sid, int x, int y, int op)
{
    SURFACE *src = sid2srf(src_sid);
    SURFACE *dst = sid2srf(dst_sid);

    if (src == 0 || dst == 0) {
        return;
    }

    GBUFFER *g = &src->g;

    g->m->blit(g, 0, 0, g->w, g->h, &dst->g, x, y, op);
}


void draw_surface2(int src_sid, int dst_sid, int op)
{
    SURFACE *src = sid2srf(src_sid);
    SURFACE *dst = sid2srf(dst_sid);

    if (src == 0 || dst == 0) {
        return;
    }

    GBUFFER *g = &src->g;

    g->m->blit(g, 0, 0, g->w, g->h, &dst->g, src->x, src->y, op);
}


/// 全体を塗りつぶす
void fill_surface(int sid, COLOR32 color)
{
    fill_rect(sid, 0, 0, 0, 0, color);
}


/// 矩形を塗りつぶす
void fill_rect(int sid, int x, int y, int w, int h, COLOR32 color)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (w == 0 && h == 0) {
        w = srf->g.w;
        h = srf->g.h;
    }

    if (srf->flags & SRF_FLG_WINDOW) {
        x += CLIENT_X;
        y += CLIENT_Y;
    }

    GBUFFER *g = &srf->g;

    g->m->fill_rect(g, x, y, w, h, color);
}


/// 文字列を画面に出力する
void draw_text(int sid, int x, int y, COLOR32 color, const char *s)
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
void draw_text_bg(int sid, int x, int y, COLOR32 color,
        COLOR32 bg_color, const char *s)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    for ( ; *s; s++) {
        x = draw_char_bg(srf, x, y, color, bg_color, *s);
    }
}


void draw_pixel(int sid, unsigned int x, unsigned int y, COLOR32 color)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    if (srf->flags & SRF_FLG_WINDOW) {
        x += CLIENT_X;
        y += CLIENT_Y;
    }

    GBUFFER *g = &srf->g;

    g->m->put(g, x, y, color);
}


/**
 * 線を引く。はりぼてOSから持ってきた
 */
void draw_line(int sid, int x0, int y0, int x1, int y1, COLOR32 color)
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

    GBUFFER *g = &srf->g;

    for (int i = 0; i < len; i++) {
        g->m->put(g, x >> 10, y >> 10, color);

        x += dx;
        y += dy;
    }
}


void erase_char(int sid, int x, int y, COLOR32 color, bool update)
{
    if (sid == 7)
        temp_dbgf("%X\n", color);

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

    if (ABS(cx) > srf->g.w || ABS(cy) > srf->g.h) {
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

    GBUFFER *g = &srf->g;

    g->m->blit(g, src_x, src_y, g->w, g->h, g, dst_x, dst_y, OP_SRC_COPY);
}


void set_colorkey(int sid, COLOR32 colorkey)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    srf->g.flags &= ~GBF_FLG_HAS_ALPHA;
    srf->g.flags |= GBF_FLG_HAS_COLORKEY;
    srf->g.colorkey = colorkey;
}


void clear_colorkey(int sid)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    srf->g.flags &= ~GBF_FLG_HAS_COLORKEY;
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

    srf->g.flags &= ~GBF_FLG_HAS_COLORKEY;
    srf->g.flags |= GBF_FLG_HAS_ALPHA;
    srf->g.alpha = alpha;
}


void clear_alpha(int sid)
{
    SURFACE *srf = sid2srf(sid);

    if (srf == 0) {
        return;
    }

    srf->g.flags &= ~GBF_FLG_HAS_ALPHA;
}


void set_mouse_pos(int x, int y)
{
    int mx = l_mouse_srf->x;
    int my = l_mouse_srf->y;

    GBUFFER *g = &l_buf_srf->g;

    g->m->blit(g, mx, my, MOUSE_W, MOUSE_H,
            &l_vram_srf->g, mx, my, OP_SRC_COPY);

    l_mouse_srf->x = x;
    l_mouse_srf->y = y;

    g = &l_mouse_srf->g;

    g->m->blit(g, 0, 0, MOUSE_W, MOUSE_H,
            &l_vram_srf->g, x, y, OP_SRC_COPY);
}


static SURFACE *l_moving_srf = 0;
static int l_moving_click_x = 0;
static int l_moving_click_y = 0;
static int l_moving_srf_x = 0;
static int l_moving_srf_y = 0;

void graphic_left_down(int x, int y)
{
    l_moving_srf = 0;

    if (l_dt_srf->num_children == 0)
        return;

    SURFACE *active_win = get_active_win();
    SURFACE *p = active_win;
    do {
        int s_x = 0;
        int s_y = 0;
        conv_screen_cord(p, &s_x, &s_y, false);

        if (s_x <= x && x < s_x + p->g.w && s_y <= y && y < s_y + p->g.h) {
            if (p != active_win) {
                remove_child(p);
                add_child(p);
            }

            // title bar
            if (y - s_y <= TITLE_BAR_HEIGHT + BORDER_WIDTH) {
                // close button
                if (x - s_x >= p->g.w - (CLOSE_BTN_W + BORDER_WIDTH)) {
                    dbgf("close button\n");
                    task_free(p->pid, 0);
                    return;
                }

                dbgf("title bar\n");
                l_moving_srf = p;
                l_moving_click_x = x;
                l_moving_click_y = y;
                l_moving_srf_x = p->x;
                l_moving_srf_y = p->y;
            }

            return;
        }
    } while ((p = p->prev_srf) != active_win);
}


void graphic_left_up(int x, int y)
{
    l_moving_srf = 0;
}


void graphic_left_drag(int x, int y)
{
    if (l_moving_srf == 0)
        return;

    int dx = x - l_moving_click_x;
    int dy = y - l_moving_click_y;

    if (dx != 0 || dy != 0) {
        move_surface(l_moving_srf->sid, l_moving_srf_x + dx, l_moving_srf_y + dy);
    }
}


SURFACE *get_active_win(void)
{
    if (l_dt_srf == 0 || l_dt_srf->num_children == 0)
        return 0;

    int i = 0;
    SURFACE *p = l_dt_srf->children;
    do {
        if (i++ > 3)
            break;
    } while ((p = p->next_srf) != l_dt_srf->children);
    return l_dt_srf->children->prev_srf;
}


void switch_window(void)
{
    // l_dt_srf->children の先頭が現在アクティブなウィンドウを表している。

    if (l_dt_srf == 0 || l_dt_srf->num_children <= 1)
        return;

    SURFACE *old_head = l_dt_srf->children;
    remove_child(old_head);
    add_child(old_head);
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
            char *s = "";
            if (srf->flags & SRF_FLG_WINDOW) {
                if (srf->flags & SRF_FLG_WIN_ACTIVE) {
                    s = "win active";
                } else {
                    s = "win";
                }
            }

            dbgf("%d: %s, buf = %p, (%d, %d, %d, %d) %s\n",
                    sid, task_get_name(srf->pid), srf->g.buf,
                    srf->x, srf->y, srf->g.w, srf->g.h, s);

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

    dbgf("\nwindows:\n");

    srf = l_dt_srf->children;
    do {
        dbgf("%d ", srf->sid);

        srf = srf->next_srf;
    } while (srf != l_dt_srf->children);

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


static void create_close_button_surface(void)
{
    static char closebtn[CLOSE_BTN_H][CLOSE_BTN_W] = {
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

    COLOR32 color_at     = 0x000000;
    COLOR32 color_dollar = 0x848484;
    COLOR32 color_q      = 0xFFFFFF;

    l_close_button_sid = new_surface(NO_PARENT_SID, CLOSE_BTN_W, CLOSE_BTN_H);
    l_close_button_srf = sid2srf(l_close_button_sid);

    GBUFFER *g = &l_close_button_srf->g;
    COLOR32 color;

    for (int y = 0; y < CLOSE_BTN_H; y++) {
        for (int x = 0; x < CLOSE_BTN_W; x++) {
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

            g->m->put(g, x, y, color);
        }
    }
}


static void create_mouse_surface(void)
{
    static char cursor[MOUSE_W * MOUSE_H] = {
        "**........"
        "*O*......."
        "*OO*......"
        "*OOO*....."
        "*OOOO*...."
        "*OOOOO*..."
        "*OOOOOO*.."
        "*OOOOOOO*."
        "*OOOOOOOO*"
        "*OOOOO****"
        "*OO*OO*..."
        "*O*.*OO*.."
        "**..*OO*.."
        ".....*OO*."
        ".....*OO*."
        "......***."
    };

    l_mouse_sid = new_surface(NO_PARENT_SID, MOUSE_W, MOUSE_H);
    l_mouse_srf = sid2srf(l_mouse_sid);
    set_colorkey(l_mouse_sid, COL_RED);

    char *cur = cursor;

    GBUFFER *g = &l_mouse_srf->g;
    COLOR32 color;

    for (int y = 0; y < MOUSE_H; y++) {
        for (int x = 0; x < MOUSE_W; x++) {
            if (*cur == '*') {
                color = COL_BLACK;
            } else if (*cur == 'O') {
                color = COL_WHITE;
            } else if (*cur == '.') {
                color = COL_RED;
            }

            g->m->put(g, x, y, color);

            cur++;
        }
    }
}


// １文字を画面に出力する
static int draw_char(SURFACE *srf, int x, int y, COLOR32 color, char ch)
{
    int old_x = x;

    if (srf->flags & SRF_FLG_WINDOW) {
        x += CLIENT_X;
        y += CLIENT_Y;
    }

    if (x + HANKAKU_W > srf->g.w || y + HANKAKU_H > srf->g.h) {
        return old_x + HANKAKU_W;
    }

    extern char hankaku[4096];
    char *font = hankaku + (((unsigned char) ch) * 16);

    GBUFFER *g = &srf->g;

    for (int ch_y = 0; ch_y < HANKAKU_H; ch_y++) {
        char font_line = font[ch_y];

        for (int ch_x = 0; ch_x < HANKAKU_W; ch_x++) {
            if (font_line & (0x80 >> ch_x)) {
                g->m->put(g, x + ch_x, y + ch_y, color);
            }
        }
    }

    return old_x + HANKAKU_W;
}


/// １文字を画面に出力する（背景色を指定）
static int draw_char_bg(SURFACE *srf, int x, int y, COLOR32 color,
        COLOR32 bg_color, char ch)
{
    int old_x = x;

    if (srf->flags & SRF_FLG_WINDOW) {
        x += CLIENT_X;
        y += CLIENT_Y;
    }

    if (x + HANKAKU_W > srf->g.w || y + HANKAKU_H > srf->g.h) {
        return old_x + HANKAKU_W;
    }

    extern char hankaku[4096];
    char *font = hankaku + (((unsigned char) ch) * 16);

    GBUFFER *g = &srf->g;

    for (int ch_y = 0; ch_y < HANKAKU_H; ch_y++) {
        char font_line = font[ch_y];

        for (int ch_x = 0; ch_x < HANKAKU_W; ch_x++) {
            if (font_line & (0x80 >> ch_x)) {
                g->m->put(g, x + ch_x, y + ch_y, color);
            } else {
                g->m->put(g, x + ch_x, y + ch_y, bg_color);
            }
        }
    }

    return old_x + HANKAKU_W;
}


static void change_win_active(void)
{
    SURFACE *srf = get_active_win();
    srf->flags |= SRF_FLG_WIN_ACTIVE;
    send_window_active_msg(g_root_pid, srf->pid);
}

static void change_win_deactive(SURFACE *srf)
{
    srf->flags &= ~SRF_FLG_WIN_ACTIVE;
    send_window_deactive_msg(g_root_pid, srf->pid);
}


static void add_child(SURFACE *srf)
{
    SURFACE *parent = srf->parent;

    if (parent == 0) {
        return;
    }

    SURFACE *old_active_win = get_active_win();

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

    if (parent == l_dt_srf) {
        if (old_active_win != 0) {
            change_win_deactive(old_active_win);
        }

        change_win_active();
    }
}


static void add_child_head(SURFACE *srf)
{
    SURFACE *parent = srf->parent;

    if (parent == 0) {
        return;
    }

    if (parent->num_children == 0) {
        parent->children = srf;

        if (parent == l_dt_srf) {
            change_win_active();
        }
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
            change_win_deactive(srf);

            send_window_active_msg(g_root_pid, ERROR_PID);
        }

        parent->children = 0;
        parent->num_children--;
        return;
    }

    SURFACE *old_active_win = get_active_win();

    SURFACE *s = parent->children;
    do {
        if (s == srf) {
            SURFACE *prev = srf->prev_srf;
            SURFACE *next = srf->next_srf;

            prev->next_srf = next;
            next->prev_srf = prev;

            if (s == parent->children) {
                parent->children = next;
            }

            if (parent == l_dt_srf && srf == old_active_win) {
                change_win_deactive(old_active_win);
                change_win_active();
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


static void conv_screen_cord(SURFACE *srf, int *x, int *y, bool client_cord)
{
    SURFACE *p = srf;

    while (p) {
        *x += p->x;
        *y += p->y;

        if (p->flags & SRF_FLG_WINDOW) {
            *x += CLIENT_X;
            *y += CLIENT_Y;
        }

        p = p->parent;
    }

    if (client_cord == false && (srf->flags & SRF_FLG_WINDOW)) {
        *x -= CLIENT_X;
        *y -= CLIENT_Y;
    }
}

