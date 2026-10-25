///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - game screen: board, robbo and status bar
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _render_h_
#define _render_h_

#include "types.h"

// the board on the 320x256 screen
#define BOARD_X         32
#define BOARD_Y         8
#define BOARD_W         256
#define BOARD_H         (VIEW_ROWS * 16)

void render_game_enter(void);
void render_game(void);

#endif
