///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - constant tables and texts from the original
//
///////////////////////////////////////////////////////////////////////////////

#include "data.h"

// the generated tile tables are defined here
#define GEN_ASSETS_DATA
#include "gen_assets.h"

const u8 level_props[LEVELS][6] = {
    { 96,  3,140,142, 12,129}, // 1
    { 96,140,  5, 12,  6,129}, // 2
    { 96,133,131,139, 11,129},
    { 96,131,140,  3,139,  1},
    {101,  5,136,  0,142,130}, // 5
    {101,  1,  5,129, 12,  0},
    {101,  5,136,  0,142,130},
    {101,  5,  3,  0,139,128},
    {114,  0,  3,139, 11,131},
    {114,133,131,128,  3,  0}, // 10
    {114,131,  2,128,142,  0},
    {114,  0,  5,  3, 11,131},
    {115,  5,  2,  3, 15,  0},
    {115,  1,133,136, 12,  0},
    {115,  0,132,140, 12,133}, // 15
    {115,  3,136,139, 11,  0},
    { 97,128,131,132, 12,  0},
    { 97,128,131,132, 12,  0},
    { 97,128,  2,  1, 12,  0},
    { 97,  3,  2,  1, 12,  0}, // 20
    {115,  0,136,140,  8,131},
    {115,  1,  3,140, 12,140},
    {115,  0,  1,140, 12,  5},
    {115,128,  5,140, 12,131},
    {102,  5,  2,140,  6,  0}, // 25
    {102,  1,  5,140,  6,  0},
    {102,  5,133,140, 12,  0},
    {102,129,  5,140, 13,131},
    { 99,  3,132, 15, 12,  0},
    { 99,  1,132, 12, 15,  0}, // 30
    { 99,  5,  3, 11,135,  0},
    { 99,131,  5,  6, 12,  0},
    { 98,  5,132,140, 12,128},
    { 98,140,  5,  6, 12,129},
    { 98,  5,131,140, 12,  0}, // 35
    { 98,  3,  5,140, 12,  0},
    {100,  0,132,140,  6,131},
    {100,  1,  5,140,  6,  0},
    {100,  0,140, 12,  6,133},
    {100,  0,132,140,  6,131}, // 40
    {104,  3,132, 12,  6,  0},
    {104,  1,  5,142,  6,  0},
    {104,133,  3,142, 12,  0},
    {104,131,133,142, 12,  0},
    {103,  3,132, 12,  6,  0}, // 45
    {103,  1,  5,140,  6,  0},
    {103,  5,132,140, 12,  0},
    {103,  3,  5,140, 12,  0},
    {112,  3,132, 12,  6,  0},
    {112,  1,  5,140,  6,  0}, // 50
    {112,140,131,140,  6,  0},
    {112,133,  3,142, 12,  0},
    {113,  3,132,140, 12,  0},
    {113,  1,  5, 12,  6,  0},
    {113,132,  3, 12,  6,  0}, // 55
    {113,  3,133, 12,  6,  0},
};

const u8 robbo_frames[4][4][2] = {
    {{52,0},{53,0},{52,0},{54,0}},
    {{52,1},{53,1},{52,1},{54,1}},
    {{50,0},{51,0},{50,0},{51,1}},
    {{48,0},{49,0},{48,0},{49,1}},
};

// what a shot question mark turns into (16 = rotating gun, 32 = eyes,
// 13 = kills every living creature)
const u8 qitems[32] = {
     0, 0,12,12, 8, 8, 8, 8,
     3, 3, 3, 1, 1, 1, 1, 1,
    32,32, 2,11,11,11,11,11,
    13,15, 5,16,16,16,16, 6,
};

const u8 title_pals[6][8] = {
    {  3,  3, 11, 11,  3,  3,  4,  4},
    {  2,  2, 13, 13, 14, 14, 13, 13},
    {  8,  8,  9,  9, 10, 10,  9,  9},
    {  1,  1,  5,  5,  6,  6,  5,  5},
    {  1,  1,  2,  2, 13, 13,  2,  2},
    {131,131,138,138,139,139,138,138},
};

const u8 blink_colours[6] = { 0, 5, 0, 6, 0, 7 };

const u8 revdir[5] = { 0, 2, 1, 4, 3 };
const u8 lpath[5]  = { 0, 4, 3, 1, 2 };
const u8 rpath[5]  = { 0, 3, 4, 2, 1 };

const u8 blast_tiles[8] = { 59, 58, 57, 56, 57, 58, 59, 0 };

// level data codes, in direction order l r u d
const u8 gun_codes[4]     = { 0x1E, 0x1F, 0x1C, 0x1D };
const u8 blaster_codes[4] = { 0x04, 0x01, 0x18, 0x17 };
const u8 laser_codes[4]   = { 0x3C, 0x3E, 0x5E, 0x27 };

const char* const guide[GUIDE_LINES] = {
    "INSTRUCTIONS:",
    "",
    "ROBBO MUST MAKE A DIFFICULT",
    "JOURNEY ACROSS 56 PLANETS TO",
    "ESCAPE FROM THE HOSTILE",
    "PLANETARY SYSTEM IN WHICH HE",
    "HAS BEEN TREACHEROUSLY",
    "IMPRISONED.",
    "",
    "ON EACH PLANET A SMALL CAPSULE",
    "IS WAITING FOR HIM. HE CAN FLY",
    "IT TO THE NEXT PLANET.",
    "",
    "UNFORTUNATELY, MOST OF THE",
    "CAPSULES ARE INCOMPLETE, SO",
    "ROBBO HAS TO COLLECT THE RIGHT",
    "NUMBER OF PARTS SCATTERED ALL",
    "OVER THE PLANET.",
    "",
    "EVIL CREATURES LIVING IN THE",
    "SYSTEM WILL TRY TO STOP HIM.",
    "",
    "ALONG THE WAY ROBBO WILL FIND",
    "ALL SORTS OF OBJECTS. DISCOVER",
    "WHAT THEY ARE FOR: TRY PUSHING,",
    "COLLECTING OR SHOOTING THEM.",
    "",
    "CONTROLS:",
    "",
    "MOVE: QAOP / CURSORS / JOYSTICK",
    "SHOOT: HOLD FIRE + DIRECTION",
    "FIRE: SPACE, M, 0 OR ENTER",
    "GIVE UP: HOLD R (LOSES A LIFE)",
    "QUIT TO TITLE: BREAK",
    "",
    "IF ROBBO FINDS HIMSELF IN A",
    "HOPELESS SITUATION, HOLD DOWN",
    "THE R KEY. ROBBO WILL LOSE ONE",
    "LIFE AND THE PLANET WILL START",
    "AGAIN.",
    "",
    "REMEMBER: EVERY PLANET CAN BE",
    "COMPLETED!",
    "",
    "ENJOY THE GAME! - THE AUTHOR",
    "",
    "THANKS TO BOCIANU, WHOSE",
    "CONVERSION OF THE ATARI 8-BIT",
    "ORIGINAL INSPIRED THIS VERSION.",
    "",
    "ZX SPECTRUM NEXT VERSION",
    "BY TecHniC, 2026.",
    "",
    "A NON-COMMERCIAL FAN CONVERSION",
    "MADE WITH THE HELP OF AI.",
    "ROBBO IS (C) 1989 LK AVALON",
    "AND JANUSZ PELC. ALL RIGHTS",
    "BELONG TO THEIR OWNERS.",
};

const char* const congrats[CONGRATS_LINES] = {
    " ROBBO HAS MADE HIS WAY THROUGH",
    "   AN ENEMY PLANETARY SYSTEM.",
    "",
    " THE PLANS HE IS CARRYING ARE",
    "     VERY VALUABLE TO EARTH!",
    "",
    "  YOU HAVE COMPLETED THE GAME",
    "",
    "\f9        CONGRATULATIONS!",
};
