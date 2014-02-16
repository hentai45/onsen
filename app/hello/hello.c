#include "onsen.h"
#include "msg.h"

void OnSenMain(void)
{
    int sid = get_screen();

    fill_surface(sid, COL_GREEN);
    draw_text(sid, 0, 0, COL_BLACK, "hello");
    update_screen(sid);

    MSG msg;

    while (get_message(&msg)) {
        if (msg.message == MSG_KEYDOWN) {
            int keycode = msg.u_param;

            if (keycode == KC_ESC) {
                exit_app(0);
            }
        }
    }

    exit_app(0);
}

