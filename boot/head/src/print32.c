#include "memory.h"
#include "sysinfo.h"

#define HANKAKU_W (8)   // 半角フォントの幅
#define HANKAKU_H (16)  // 半角フォントの高さ


extern struct SYSTEM_INFO *g_sys_info;

typedef unsigned short COLOR16;

static const COLOR16 fg = 0xFFFF;
static const COLOR16 bg = 0x0000;


static void draw_char(int x, int y, char ch);

// 文字列を画面に出力する
void print32(const char *s)
{
    int x = 8;
    int y = 8;

    int old_x = x;

    for ( ; *s; s++) {
        if (*s == '\n') {
            x = old_x;
            y += HANKAKU_H;
        } else {
            draw_char(x, y, *s);
            x += HANKAKU_W;
        }
    }
}


// １文字を画面に出力する
static void draw_char(int x, int y, char ch)
{
    extern char hankaku[4096];
    char *font = hankaku + (((unsigned char) ch) * 16);

    COLOR16 *vram = (COLOR16 *) VADDR_VRAM;

    for (int ch_y = 0; ch_y < HANKAKU_H; ch_y++) {
        char font_line = font[ch_y];

        for (int ch_x = 0; ch_x < HANKAKU_W; ch_x++) {
            COLOR16 *g = &vram[(y + ch_y) * g_sys_info->w + (x + ch_x)];

            if (font_line & (0x80 >> ch_x)) {
                *g = fg;
            } else {
                *g = bg;
            }
        }
    }
}

