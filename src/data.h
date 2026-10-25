///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - constant tables and texts from the original
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _data_h_
#define _data_h_

#include "types.h"

#define LEVELS 56

// per level: wall tile, background, wall dark, wall light, wall highlight,
// crate colour (base colours, 128+ is the secret palette)
extern const u8 level_props[LEVELS][6];

// robbo frames per direction (l r u d) and step: tile, horizontal flip
extern const u8 robbo_frames[4][4][2];

extern const u8 qitems[32];
extern const u8 title_pals[6][8];
extern const u8 blink_colours[6];

// direction tables, indexed by direction 1-4 (l r u d)
extern const u8 revdir[5];
extern const u8 lpath[5];
extern const u8 rpath[5];

extern const u8 blast_tiles[8];
extern const u8 gun_codes[4];
extern const u8 blaster_codes[4];
extern const u8 laser_codes[4];

#define GUIDE_LINES 58
extern const char* const guide[GUIDE_LINES];

#define CONGRATS_LINES 9
extern const char* const congrats[CONGRATS_LINES];

#endif
