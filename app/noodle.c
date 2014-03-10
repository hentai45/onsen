// haribote OS APP

#include "onsen.h"

void draw_time(int sid, int hou, int min, int sec);

#define bg (0xFFFFFF)
#define fg (0)

void main(void)
{

    int sid = create_window(142, 22, "noodle");

    int tid = timer_new();
    timer_start(tid, 1000);

    int sec = 0, min = 0, hou = 0;

    draw_time(sid, hou, min, sec);

    struct MSG msg;

    while (get_message(&msg)) {
        if (msg.message == MSG_KEYDOWN) {
            int keycode = msg.u_param;

            if (keycode == KC_ENTER) {
                exit(0);
            }
        }

        if (msg.message == MSG_TIMER) {
            draw_time(sid, hou, min, sec);
            timer_start(tid, 1000);

            sec++;
            if (sec == 60) {
                sec = 0;
                min++;
                if (min == 60) {
                    min = 0;
                    hou++;
                }
            }
        }
    }

    exit(0);
}

void draw_time(int sid, int hou, int min, int sec)
{
    static char s[12];
    snprintf(s, 12, "%5d:%02d:%02d", hou, min, sec);
    fill_surface(sid, bg);
    draw_text(sid, 20, 3, 0, s);
    update_surface(sid);
}
