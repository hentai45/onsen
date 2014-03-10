// haribote OS APP


#include "onsen.h"

static unsigned int l_color_tbl[] = {
    0x000000,
    0xFF0000,
    0x00FF00,
    0xFFFF00,
    0x0000FF,
    0xFF00FF,
    0x00FFFF,
    0xFFFFFF,
    0xC6C6C6,
    0x840000,
    0x008400,
    0x848400,
    0x000084,
    0x840084,
    0x008484,
    0x848484
};

void main(void)
{
    static int tbl[16][2] = {
        { 196, 101 }, { 187,  62 }, { 164,  30 }, { 129,  10 }, {  90,   6 },
        {  53,  18 }, {  23,  45 }, {   7,  82 }, {   7, 120 }, {  23, 157 },
        {  53, 184 }, {  90, 196 }, { 129, 192 }, { 164, 172 }, { 187, 140 },
        { 196, 101 }
    };

    int sid = create_window(200, 200, "bball");
    fill_surface(sid, 0);
    int dist;

    for (int i = 0; i <= 14; i++) {
        for (int j = i + 1; j <= 15; j++) {
            dist = j - i;

            if (dist >= 8) {
                dist = 15 - dist;
            }

            if (dist != 0) {
                draw_line(sid, tbl[i][0], tbl[i][1], tbl[j][0], tbl[j][1], l_color_tbl[8 - dist]);
            }
        }
    }

    update_surface(sid);

    for (;;) {
        if (getkey(1) == '\n')
            break;
    }

    exit(0);
}
