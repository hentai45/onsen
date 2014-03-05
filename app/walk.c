// haribote OS APP

#include "onsen.h"

#define W (160)
#define H (80)

void main(void)
{
    int sid = create_window(W, H, "walk");
    int x = W / 2;
    int y = H / 2;
    unsigned int bg = 0;
    unsigned int fg = 0xFFFF00;

    fill_surface(sid, bg);
    draw_text(sid, x, y, fg, "*");

    for (;;) {
        update_surface(sid);

        int ch = getkey(1);

        if (ch == '\n')
            break;

        draw_text(sid, x, y, bg, "*");
        if (ch == '4' && x > 0)       { x -= 8; }
        if (ch == '6' && x < W - 8)   { x += 8; }
        if (ch == '8' && y > 0)       { y -= 8; }
        if (ch == '2' && y < H - 16)  { y += 8; }
        draw_text(sid, x, y, fg, "*");
    }

    exit(0);
}
