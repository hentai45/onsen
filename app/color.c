// haribote OS APP

#include "onsen.h"

void main(void)
{
    int sid = create_window(128, 128, "color");

    for (int y = 0; y < 128; y ++) {
        for (int x = 0; x < 128; x++) {
            unsigned int color = RGB(x*2, y*2, 0);
            draw_pixel(sid, x, y, color);
        }
    }

    update_surface(sid);

    getkey(1);

    exit(0);
}
