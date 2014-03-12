#include "myc.h"
#include "onsen.h"
#include "msg.h"

#define TIME_MS 100
#define TEXT  "hello"
#define MAX_X 300
#define MAX_Y 200

void main(void)
{
    int sid = create_window(MAX_X, MAX_Y, TEXT);
    int x = 0, y = 0;
    int step_x = 4, step_y = 0;

    printf("printf: %d %s\n", 32, "test");

    draw_text(sid, x, y, 0, TEXT);
    update_surface(sid);

    int tid = timer_new();
    timer_start(tid, TIME_MS);

    struct MSG msg;

    while (get_message(&msg)) {
        if (msg.message == MSG_KEYDOWN) {
            if (msg.u_param == KC_ENTER) {
                exit(0);
            }
        }

        if (msg.message == MSG_TIMER) {
            fill_surface(sid, 0xFFFFFF);

            x += step_x;
            if (x >= MAX_X) {
                step_x = -step_x;
            } else if (x == 0) {
                step_x = -step_x;
            }

            y += step_y;
            if (y >= MAX_Y) {
                step_y = -step_y;
            } else if (y == 0) {
                step_y = -step_y;
            }

            draw_text(sid, x, y, 0, TEXT);
            update_surface(sid);
            timer_start(tid, TIME_MS);
        }
    }

    exit(0);
}

