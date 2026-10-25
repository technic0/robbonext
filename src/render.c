///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - game screen: board, robbo and status bar
//
//  Screen layout (320x256):
//      y   8..231  board: 16 x 14 tiles of 16x16 at x 32..287
//      y 236..251  status bar
//
//  Layer 2 (320x256, 256 colours, stored column by column) holds a ring of
//  16 map rows. A cell is redrawn only when the tile graphics shown in it
//  change; scrolling uses the layer 2 Y offset, and a clip window keeps the
//  board in its frame. Pixels are colour * 8 + shade, so each level only
//  reloads the shades of the colours it swaps.
//
//  The status bar is made of sprites; its numbers are drawn into sprite
//  patterns when they change.
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "render.h"
#include "data.h"
#include "fx.h"
#include "game.h"
#include "gen_assets.h"
#include "hw.h"

#define RING_ROWS       16      // 256 lines of layer 2

#define HUD_Y           236

#define SLOT_ROBBO      0
#define SLOT_PANEL      90      // 18 sprites
#define SLOT_ICONS      108     // 5
#define SLOT_DIGITS     113     // 9

#define PAT_DIGITS      32      // patterns drawn at run time (after the set)

static u8 shown[RING_ROWS][MAP_W];      // slot drawn in each ring cell
static u8 shown_level;
static u8 shown_bg;
static u8 shown_curtain;

// map tile -> slot drawn now (two frame swaps and animation frames)
static u8   slot_map[128];
static bool slot_anim;
static bool slot_ship;
static u8   slot_phase;
static u8   slot_version;               // changes when slot_map does

// what the board was last drawn from: nothing to do when all are the same
static u8   drawn_version;
static u8   drawn_slots;
static s8   drawn_first;

static const u8 empty_row[MAP_W];

///////////////////////////////////////////////////////////////////////////////
// palettes

static void apply_level_palette(void)
{
    const u8* p = level_props[level - 1];
    u8 i;
    for (i = 0; i < 16; ++i) {
        u8 c = i;
        // the colours each level swaps
        if (i == 1) c = p[1];
        else if (i == 2) c = p[2];
        else if (i == 14) c = p[3];
        else if (i == 15) c = p[4];
        else if (i == 4) c = p[5];
        pal_ramp(PAL_LAYER2, i, c);
        pal_ramp(PAL_SPRITES, i, c);
    }
    shown_level = level;
    shown_bg = 0xFF;
}

///////////////////////////////////////////////////////////////////////////////
// board

static void build_slot_map(void)
{
    u8 t;
    for (t = 0; t < 128; ++t) {
        u8 d = t;
        u8 n;
        // flipsprites(): animated tiles swap with their second frame
        if (swap_anim) {
            if ((t >= 30 && t <= 47) || t == 60 || t == 61) d = t ^ 1;
            else if (t == 119) d = 120;
            else if (t == 120) d = 119;
        }
        if (swap_ship && (t == T_SHIP || t == T_SHIP_ALT)) d = t ^ 1;
        // tiles may have more frames of their own
        n = tile_frames[d];
        if (n > 1) {
            u8 f = slot_phase & (n - 1);        // frame counts are 4 or 8
            slot_map[t] = f ? tile_frame_slot[d] + f - 1 : tile_slots[d];
        } else {
            slot_map[t] = tile_slots[d];
        }
    }
    slot_anim = swap_anim;
    slot_ship = swap_ship;
    ++slot_version;
}

static void render_board(s16 cam)
{
    s8 first = cam >> 4;
    u8 k, c;
    u8 src_page = 0xFF;
    u8 phase = (u8)(frame / 3);

    if (slot_anim != swap_anim || slot_ship != swap_ship || slot_phase != phase) {
        slot_phase = phase;
        build_slot_map();
    }
    // world row r is at layer 2 line (r * 16) % 256, shown at BOARD_Y
    layer2_scroll_y((u8)(cam - BOARD_Y));

    if (drawn_version == map_version && drawn_slots == slot_version &&
        drawn_first == first) return;
    drawn_version = map_version;
    drawn_slots = slot_version;
    drawn_first = first;

    for (k = 0; k <= VIEW_ROWS; ++k) {
        s8 r = first + k;
        u8 ring = (u8)r & (RING_ROWS - 1);
        const u8* row = (r >= 0 && r < MAP_ROWS) ? (const u8*)map[r] : empty_row;
        u8* cache = shown[ring];
        u8 dst_page = 0xFF;
        for (c = 0; c < MAP_W; ++c) {
            u8 t = *row++ & 127;
            u8 slot = slot_map[t];
            u8 page;
            if (*cache == slot) {
                ++cache;
                continue;
            }
            *cache++ = slot;
            // layer 2 columns: 32 per page, the board starts at x = 32
            page = PAGE_LAYER2 + 1 + (c >> 1);
            if (page != dst_page) {
                page_in(page);
                dst_page = page;
            }
            // tile graphics, 32 per page, mapped at 0x4000 over bank 5
            page = PAGE_TILES + (slot >> 5);
            if (page != src_page) {
                nextreg(0x52, page);
                src_page = page;
            }
            blit_tile((const u8*)0x4000 + ((u16)(slot & 31) << 8),
                      (u8*)0x2000 + ((c & 1) << 12) + (ring << 4));
        }
    }
    if (src_page != 0xFF) nextreg(0x52, PAGE_BANK5);
    page_out();
}

///////////////////////////////////////////////////////////////////////////////
// status bar

static const u8 icon_sprites[5] = {
    SPR_ICON_SCREW, SPR_ICON_LIFE, SPR_ICON_KEY, SPR_ICON_AMMO, SPR_ICON_LEVEL
};

static u32 shown_score;
static u8  shown_screws;
static s8  shown_lives;
static u8  shown_keys;
static u16 shown_ammo;
static u8  shown_level_no;
static u8  digit_buf[256];

// draw two characters ('0'-'9' or space) into a 16x16 sprite pattern
static void hud_pair(u8 index, char a, char b)
{
    const u8* glyphs = PAGE_WINDOW + MISC_DIGITS;
    u8 y;
    page_in(PAGE_MISC);
    for (y = 0; y < 16; ++y) {
        u8* d = digit_buf + y * 16;
        if (a == ' ') memset(d, 0, 8);
        else memcpy(d, glyphs + (a - '0') * 128 + y * 8, 8);
        if (b == ' ') memset(d + 8, 0, 8);
        else memcpy(d + 8, glyphs + (b - '0') * 128 + y * 8, 8);
    }
    page_out();
    sprite_pattern(PAT_DIGITS + index, digit_buf);
}

// number as text, zero padded to 'digits', at most 6 digits
static void number(char* out, u32 v, u8 digits)
{
    char tmp[7];
    u8 n = 0;
    do {
        tmp[n++] = '0' + (u8)(v % 10);
        v /= 10;
    } while (v && n < 6);
    while (n < digits) tmp[n++] = '0';
    while (n) *out++ = tmp[--n];
    *out = 0;
}

static void hud_two(u8 index, u16 v)
{
    char s[7];
    number(s, v > 99 ? 99 : v, 2);
    hud_pair(index, s[0], s[1]);
}

static void render_status(void)
{
    char s[7];
    if (shown_score != score) {
        shown_score = score;
        number(s, score, 6);
        hud_pair(0, s[0], s[1]);
        hud_pair(1, s[2], s[3]);
        hud_pair(2, s[4], s[5]);
    }
    if (shown_screws != rscrews) {
        shown_screws = rscrews;
        hud_two(3, rscrews);
    }
    if (shown_lives != rlives) {
        shown_lives = rlives;
        hud_two(4, rlives < 0 ? 0 : rlives);
    }
    if (shown_keys != rkeys) {
        shown_keys = rkeys;
        hud_two(5, rkeys);
    }
    if (shown_ammo != rammo) {
        // at least two digits, up to three
        shown_ammo = rammo;
        number(s, rammo > 999 ? 999 : rammo, 2);
        hud_pair(6, s[0], s[1]);
        hud_pair(7, s[2] ? s[2] : ' ', ' ');
    }
    if (shown_level_no != level) {
        shown_level_no = level;
        hud_two(8, level);
    }
}

static void status_bar(void)
{
    // positions of the 9 number sprites: score (3), screws, lives, keys,
    // ammo (2), level
    static const u16 digit_x[9] = { 20, 36, 52, 100, 144, 188, 232, 248, 276 };
    u8 i;
    for (i = 0; i < 18; ++i) {
        u8 pat = i == 0 ? SPR_PANEL_L : i == 17 ? SPR_PANEL_R : SPR_PANEL_M;
        sprite_show(SLOT_PANEL + i, 16 + i * 16, HUD_Y, pat, 0, 0);
    }
    for (i = 0; i < 5; ++i)
        sprite_show(SLOT_ICONS + i, 84 + i * 44, HUD_Y, icon_sprites[i], 0, 0);
    for (i = 0; i < 9; ++i)
        sprite_show(SLOT_DIGITS + i, digit_x[i], HUD_Y, PAT_DIGITS + i, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
// curtain: the board is clipped from the left

static void render_board_clip(u8 cols)
{
    u16 x1;
    if (cols == shown_curtain) return;
    shown_curtain = cols;
    if (cols >= 16) {
        layer2_enable(false);
        return;
    }
    x1 = BOARD_X + cols * 16;
    layer2_clip(x1, BOARD_X + BOARD_W - 1, BOARD_Y, BOARD_Y + BOARD_H - 1);
    layer2_enable(true);
}

///////////////////////////////////////////////////////////////////////////////

void render_game_enter(void)
{
    sprite_hide_all();
    sprite_clip(0, 319, 0, 255);
    sprite_load_set(SET_GAME);
    pal_default(PAL_LAYER2);
    pal_default(PAL_SPRITES);
    memset(shown, 0xFF, sizeof(shown));
    slot_phase = 0xFF;
    drawn_first = -128;
    shown_level = 0;
    shown_curtain = 0xFF;

    tilemap_enable(false);

    shown_score = 0xFFFFFFFF;
    shown_screws = 0xFF;
    shown_lives = -128;
    shown_keys = 0xFF;
    shown_ammo = 0xFFFF;
    shown_level_no = 0xFF;
    status_bar();
    fx_clear();
}

void render_game(void)
{
    s16 cam = map_ypos * 16 - map_yoff * 2;
    u8 cols = curtain > 16 ? 16 : curtain;

    if (shown_level != level) apply_level_palette();
    if (shown_bg != bg_colour) {
        shown_bg = bg_colour;
        pal_ramp(PAL_LAYER2, 1, bg_colour);
    }

    render_board(cam);
    render_board_clip(cols);

    if (robbo_visible && cols == 0) {
        s16 sy = BOARD_Y + ry * 16 - cam;
        if (sy >= BOARD_Y && sy <= BOARD_Y + BOARD_H - 16)
            sprite_show(SLOT_ROBBO, BOARD_X + (rx2 >> 1) * 16, sy,
                        SPR_ROBBO_48 + robbo_frame - 48, robbo_flip ? SPR_MIRROR_X : 0, 0);
        else
            sprite_hide(SLOT_ROBBO);
    } else {
        sprite_hide(SLOT_ROBBO);
    }

    fx_draw(cam, cols);
    render_status();
}
