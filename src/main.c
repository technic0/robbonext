///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - Robbo for the ZX Spectrum Next
//
//  Port of the Atari demake by bocianu. The original game runs at
//  30 updates per second; here one update runs every other frame at 60 Hz,
//  or three times every five frames at 50 Hz. Sound runs every frame.
//
//  Each update frame starts by drawing the result of the previous update
//  while the beam is in the bottom border, then runs the game logic, which
//  does not touch the screen and may overlap the visible display.
//
///////////////////////////////////////////////////////////////////////////////

#include "game.h"
#include "hw.h"
#include "render.h"
#include "sound.h"
#include "title.h"

int main(void)
{
    u8 shown_state = 0xFF;
    u8 ticks = 0;
    bool hz60;

    hw_init();
    hz60 = hw_is_60hz();
    sound_init(hz60);
    show_splash();

    for (;;) {
        hw_wait_frame();

        ticks += hz60 ? 1 : 3;
        if (ticks < (hz60 ? 2 : 5)) {
            sound_update();
            continue;
        }
        ticks -= hz60 ? 2 : 5;

        // draw
        switch (gamestate) {
        case STATE_TITLE:
            title_draw();
            break;
        case STATE_GAME:
            if (shown_state != STATE_GAME) render_game_enter();
            render_game();
            break;
        case STATE_ENDING:
            ending_draw();
            break;
        default:
            break;
        }
        shown_state = gamestate;
        sound_update();

        // update
        input_update();
        switch (gamestate) {
        case STATE_TITLE:  title_update(); break;
        case STATE_GAME:   game_update(); break;
        case STATE_ENDING: ending_update(); break;
        case STATE_SPLASH: splash_update(); break;
        }
        if (gamestate == STATE_GAME) game_draw();
        ++frame;
    }
    return 0;
}
