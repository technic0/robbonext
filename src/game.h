///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - game logic, ported from the version by bocianu
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _game_h_
#define _game_h_

#include "types.h"

#define MAP_W       16
#define MAP_ROWS    32      // 31 playable rows + one row of void
#define VIEW_ROWS   14      // rows visible on screen (10 in the original)
#define LAST_LEVEL  56

// tiles with a special meaning
#define T_EMPTY     0
#define T_SCREW     1
#define T_BOMB      2
#define T_KEY       3
#define T_DOOR      4
#define T_AMMO      8
#define T_MAGNET_L  9
#define T_MAGNET_R  10
#define T_LIFE      11
#define T_QUESTION  12
#define T_SHIP      14
#define T_SHIP_ALT  15
#define T_TELEPORT  46
#define T_TELEPORT2 47
#define T_ROBBO     62
#define T_VOID      63
#define T_BARRIER   119

enum { STATE_TITLE, STATE_GAME, STATE_ENDING, STATE_SPLASH };

extern u8   gamestate;
extern u16  frame;
extern bool cheat_mode;

// state the renderer reads
extern u8   map[MAP_ROWS][MAP_W];
extern u8   map_version;    // changes whenever the map does (for drawing)
extern u8   level;
extern s8   rlives;
extern u32  score;
extern u8   rscrews;
extern u8   rkeys;
extern u16  rammo;
extern u8   rx2;            // robbo x in half cells (magnets pull by half)
extern u8   ry;
extern u8   robbo_frame;
extern bool robbo_flip;
extern bool robbo_visible;
extern s8   map_ypos;
extern s8   map_yoff;       // 8 per row (2 pixels each), -6..6
extern u8   curtain;
extern u8   blink_back;
extern u8   bg_colour;      // background, flashes after the last screw
extern bool swap_anim;      // animated tiles show their second frame
extern bool swap_ship;      // ship blinks once all screws are collected

u8   rnd(u8 n);
void rnd_seed(u16 seed);

void game_start(void);      // new game from level 1
void game_update(void);     // one game update (30 Hz)
void game_draw(void);       // the timed events the original ran from _draw

// provided by title.c
void show_splash(void);
void show_title(void);
void start_ending(void);

#endif
