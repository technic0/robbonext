///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - title screen and ending
//
//  Both screens are a 320x256 layer 2 picture (art/hd/title.png and
//  ending.png) with sprites on top. The instructions scroll on the
//  tilemap, which moves smoothly with its own scroll register; other text
//  is drawn straight into layer 2.
//
//  This code runs from page 47 (see the makefile).
//
///////////////////////////////////////////////////////////////////////////////

#include "title.h"
#include "data.h"
#include "game.h"
#include "gen_assets.h"
#include "hw.h"
#include "sound.h"

extern u8 font_data[];

///////////////////////////////////////////////////////////////////////////////
// text in layer 2

// draw text at (x, y) of the 320x256 screen in colour 'colour' (shaded from
// light at the top to the colour at the bottom) on index 'bg', or with no
// background when bg is 0; "\f" and a hex digit change the colour
static void l2_print(u16 x, u8 y, const char* s, u8 colour, u8 bg)
{
    u8 page = 0xFF;
    for (; *s; ++s) {
        const u8* glyph;
        u8 cx;
        if (*s == '\f') {
            char h = *++s;
            colour = (h >= 'a') ? h - 'a' + 10 : h - '0';
            continue;
        }
        glyph = font_data + (u8)(*s - 32) * 8;
        for (cx = 0; cx < 8; ++cx, ++x) {
            u8* col;
            u8 bit = 0x80 >> cx;
            u8 r;
            if (x >= 320) return;
            if ((x >> 5) + PAGE_LAYER2 != page) {
                page = (x >> 5) + PAGE_LAYER2;
                page_in(page);
            }
            col = (u8*)0x2000 + ((x & 31) << 8) + y;
            for (r = 0; r < 8; ++r) {
                if (glyph[r] & bit) col[r] = colour * 8 + 6 - (r >> 1);
                else if (bg) col[r] = bg;
            }
        }
    }
    page_out();
}

static void l2_print_centred(u8 y, const char* s, u8 colour, u8 bg)
{
    u8 n = 0;
    const char* p;
    for (p = s; *p; ++p) {
        if (*p == '\f') ++p;
        else ++n;
    }
    l2_print(160 - n * 4, y, s, colour, bg);
}

///////////////////////////////////////////////////////////////////////////////
// opening picture, before the title screen

static bool splash_armed;       // all keys were released since it appeared

void show_splash(void)
{
    gamestate = STATE_SPLASH;
    frame = 0;
    splash_armed = false;
    sprite_hide_all();
    tilemap_enable(false);
    layer2_enable(false);
    layer2_scroll_y(0);
    layer2_clip(0, 319, 0, 255);
    layer2_picture(PAGE_SPLASH_PIC);
    pal_load(PAL_LAYER2, MISC_SPLASH_PAL);
    layer2_enable(true);
    music_play(0);
}

// any key or joystick fire goes on to the title screen
void splash_update(void)
{
    bool key = any_key();
    if (!splash_armed) {
        splash_armed = !key;
        return;
    }
    if (key) {
        show_title();
        // the key that left the picture must not start a game too
        btn_state = 0xFF;
        btn_pressed = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
// title

#define GUIDE_TOP       108     // the window in title.png
#define GUIDE_BOTTOM    220
#define GUIDE_COL       5
#define GUIDE_LENGTH    ((GUIDE_BOTTOM - GUIDE_TOP) + GUIDE_LINES * 8)

#define FOOTER_Y        232
#define BAND            (30 * 8 + 0)    // colour of the band behind it

static u8   titlepal;
static u16  paloff;
static s8   paldir;
static u8   palcounter;
static u16  guide_y10;          // scroll position in tenths of a pixel
static s16  guide_written;      // last guide line put in the tilemap
static bool c_was_down;

// the Atari version's secret: typing "janusz pelc" on the title screen
// starts the game with 80 lives
#define CODE_LENGTH 11
static const u16 code_keys[CODE_LENGTH] = {
    0xBF08, 0xFD01, 0x7F08, 0xDF08, 0xFD02, 0xFE02,    // J A N U S Z
    KEY_SPACE,
    0xDF01, 0xFB04, 0xBF02, 0xFE08,                    // P E L C
};
static u8   code_pos;           // letters typed so far
static u16  code_was_down;      // keys of the code held last update
static bool code_done;

static const u8 logo[5] = { SPR_LOGO_R, SPR_LOGO_O, SPR_LOGO_B, SPR_LOGO_B, SPR_LOGO_O };

static void title_footer(void)
{
    l2_print(0, FOOTER_Y, "                                        ", 0, BAND);
    if (cheat_mode) l2_print_centred(FOOTER_Y, "\f8C\f9H\faE\fbA\fcT\fd!", 8, BAND);
}

static void title_logo(void)
{
    u8 i;
    for (i = 0; i < 5; ++i) sprite_show_big(1 + i * 4, 52 + i * 44, 12, logo[i], 2, 2);
}

// the tilemap palette: 1 = text, 2 = shadow; palette 1 white, 2 yellow
static void guide_palettes(void)
{
    pal_set_rgb9(PAL_TILEMAP, 0x11, ramp_rgb9(6, 4));
    pal_set_rgb9(PAL_TILEMAP, 0x12, ramp_rgb9(0, 1));
    pal_set_rgb9(PAL_TILEMAP, 0x21, ramp_rgb9(10, 5));
    pal_set_rgb9(PAL_TILEMAP, 0x22, ramp_rgb9(0, 1));
}

// put guide line i (blank past the end) into tilemap row i % 32
static void guide_line(s16 i)
{
    u8* e = TILEMAP + ((i & 31) * TILEMAP_W) * 2;
    const char* s = (i >= 0 && i < GUIDE_LINES) ? guide[i] : "";
    // headings (ending with ':') in yellow
    u8 attr = 0x10;
    u8 c;
    {
        const char* p = s;
        while (*p) ++p;
        if (p != s && p[-1] == ':') attr = 0x20;
    }
    for (c = 0; c < TILEMAP_W; ++c) {
        u8 ch = 0;
        if (c >= GUIDE_COL && *s) ch = (u8)(*s++ - 32);
        *e++ = ch;
        *e++ = attr;
    }
}

static void guide_reset(void)
{
    s16 i;
    guide_y10 = 0;
    for (i = 0; i < 32; ++i) guide_line(i - 32);
    guide_written = -1;
}

// showTitle()
void show_title(void)
{
    titlepal = 0;
    paloff = 1;
    paldir = 1;
    palcounter = 0;
    map_ypos = 0;
    map_yoff = 0;
    frame = 0;
    curtain = 0;
    blink_back = 0;
    c_was_down = true;
    code_pos = 0;
    code_was_down = 0xFFFF;
    code_done = false;
    gamestate = STATE_TITLE;

    sprite_hide_all();
    sprite_clip(0, 319, 0, 255);
    sprite_load_set(SET_TITLE);
    pal_default(PAL_LAYER2);
    pal_default(PAL_SPRITES);

    layer2_enable(false);
    layer2_scroll_y(0);
    layer2_clip(0, 319, 0, 255);
    layer2_picture(PAGE_TITLE_PIC);
    title_footer();
    layer2_enable(true);

    tilemap_enable(false);
    tilemap_patterns(MISC_TM_FONT, 96 * 32);
    tilemap_clear();
    guide_palettes();
    guide_reset();
    tilemap_clip(16, 303, GUIDE_TOP, GUIDE_BOTTOM - 1);
    tilemap_enable(true);

    title_logo();
    // the music started on the opening picture keeps playing
    if (!music_playing()) music_play(0);
}

// updatetitle()
void title_update(void)
{
    bool c_down = key_down(KEY_C);
    bool typing = false;        // the key pressed now belongs to the code
    u16 down = 0, pressed;
    u8 i;

    // "janusz pelc": follow the keys of the code as they are pressed
    for (i = 0; i < CODE_LENGTH; ++i)
        if (key_down(code_keys[i])) down |= 1 << i;
    pressed = down & ~code_was_down;
    code_was_down = down;
    if (pressed && !code_done) {
        // letters repeat in the code (A, S...): test the expected one first
        if (pressed & (1 << code_pos)) {
            typing = true;
            if (++code_pos == CODE_LENGTH) {
                code_done = true;
                sfx(SFX_EXTRA_LIFE);
            }
        } else {
            code_pos = (pressed & 1) ? 1 : 0;     // J starts again
        }
    }

    if (palcounter > 0) {
        --palcounter;
    } else {
        paldir = rnd(2) ? 1 : -1;
        titlepal = rnd(6);
        palcounter = 120;
    }
    paloff += paldir;

    // cheat mode toggle (not while C is typed as the last letter of the
    // code)
    if (c_down && !c_was_down && !typing) {
        cheat_mode = !cheat_mode;
        sfx(SFX_EXTRA_LIFE);
        title_footer();
    }
    c_was_down = c_down;

    // space is also fire, except as the space in "janusz pelc"
    if ((btn_pressed & BTN_FIRE) && !typing) {
        game_start();
        if (code_done) rlives = 80;
    }
}

static void draw_guide(void)
{
    u16 pos = guide_y10 / 10;
    s16 last = pos / 8 + 1;
    // lines are written into the tilemap ring just before they show
    while (guide_written < last) guide_line(++guide_written);
    // line i sits at tilemap y i * 8 and must show at GUIDE_BOTTOM + i * 8 - pos
    tilemap_scroll_y((u8)(pos - GUIDE_BOTTOM));
}

// drawtitle()
void title_draw(void)
{
    const u8* cpal = title_pals[titlepal];
    u8 c;
    // the logo is drawn in colours 8-15, cycled as in the original
    for (c = 0; c < 8; ++c)
        pal_ramp(PAL_SPRITES, 8 + c, cpal[((paloff >> 2) + c) & 7]);

    draw_guide();
    guide_y10 += 3;
    if (guide_y10 / 10 > GUIDE_LENGTH) guide_reset();
}

///////////////////////////////////////////////////////////////////////////////
// ending

#define SLOT_ROBBO  0       // 2x2 sprites
#define SLOT_UFO    4       // 4x2
#define SLOT_FEET   12      // 2x1

static s16  robbo_x;            // scene pixels (half size)
static u8   robbo_spr;
static s16  ufo_y10;            // scene pixels * 10
static s16  ufo_dy10;
static u8   light_a, light_b;
static bool congrats_shown;

// scene coordinates (128x128) to the 320x256 screen
#define SCENE_X(x) (32 + (s16)(x) * 2)
#define SCENE_Y(y) ((s16)(y) * 2)

static void hide_slots(u8 slot, u8 n)
{
    while (n--) sprite_hide(slot++);
}

// sprite number of the original sheet to sprite pattern
static u8 big_sprite(u8 n)
{
    switch (n) {
    case 74:  return SPR_BIG_74;
    case 76:  return SPR_BIG_76;
    case 78:  return SPR_BIG_78;
    case 106: return SPR_BIG_106;
    default:  return SPR_BIG_108;
    }
}

// the curtain uncovers the screen from the left
static void ending_clip(void)
{
    u16 x1 = curtain >= 16 ? 319 : curtain * 20;
    layer2_clip(x1, 319, 0, 255);
    sprite_clip(x1, 319, 0, 255);
}

// startEnding()
void start_ending(void)
{
    gamestate = STATE_ENDING;
    robbo_x = 138;
    robbo_spr = 76;
    ufo_y10 = 750;
    ufo_dy10 = 10;
    frame = 0;
    curtain = 16;
    light_a = 9;
    light_b = 10;
    congrats_shown = false;

    sprite_hide_all();
    sprite_load_set(SET_ENDING);
    pal_default(PAL_LAYER2);
    pal_default(PAL_SPRITES);
    pal_ramp(PAL_SPRITES, 2, 133);
    pal_ramp(PAL_SPRITES, 15, 140);

    tilemap_enable(false);
    layer2_enable(false);
    layer2_scroll_y(0);
    layer2_picture(PAGE_ENDING_PIC);
    ending_clip();
    layer2_enable(true);
}

// updateending()
void ending_update(void)
{
    if (curtain > 0) --curtain;
    if (btn_pressed & BTN_FIRE) show_title();
}

static void walk(void)
{
    robbo_x -= 4;
    robbo_spr = (robbo_spr == 76) ? 78 : 76;
    sfx(SFX_STEP);
}

// drawending()
void ending_draw(void)
{
    bool keyframe = (frame & 3) == 0;
    s16 uy;
    u8 i, shown = 0;

    ending_clip();

    // the stars twinkle: their shades of the ice colour rotate
    if (keyframe) pal_ramp(PAL_LAYER2, 26, (frame & 4) ? 26 : 17);

    // robbo walks to the capsule, waves and gets in
    if (frame < 68) {
        shown = robbo_spr;
        if (keyframe) walk();
    } else if (frame < 100) {
        shown = 74;
        robbo_spr = 106;
    } else if (frame < 140) {
        shown = robbo_spr;
        if (keyframe) {
            robbo_spr = (robbo_spr == 106) ? 108 : 106;
            sfx(SFX_PUFF);
        }
    } else if (frame < 150) {
        shown = 74;
        robbo_spr = 76;
    } else if (frame < 180) {
        shown = robbo_spr;
        if (keyframe) walk();
    }
    if (shown) sprite_show_big(SLOT_ROBBO, SCENE_X(robbo_x), SCENE_Y(75), big_sprite(shown), 2, 2);
    else hide_slots(SLOT_ROBBO, 4);

    // the capsule takes off
    if (frame == 180) sfx(SFX_EXTRA_LIFE);
    if (frame == 190) sfx(SFX_LANDING);
    if (frame < 180) {
        sprite_show_big(SLOT_FEET, SCENE_X(44), SCENE_Y(75), SPR_UFO_FEET, 2, 1);
    } else {
        hide_slots(SLOT_FEET, 2);
        if (frame < 190) {
            ufo_dy10 = 10;
        } else if (frame < 220) {
            ufo_y10 -= ufo_dy10;
            ufo_dy10 += 2;
        }
    }
    uy = SCENE_Y(ufo_y10 / 10);
    if (uy > -32) sprite_show_big(SLOT_UFO, SCENE_X(36), uy, SPR_UFO, 4, 2);
    else hide_slots(SLOT_UFO, 8);

    // blinking capsule lights
    if (keyframe) {
        u8 c = light_a;
        light_a = light_b;
        light_b = c;
        pal_ramp(PAL_SPRITES, 9, light_a);
        pal_ramp(PAL_SPRITES, 10, light_b);
    }

    if (frame > 240 && !congrats_shown) {
        congrats_shown = true;
        for (i = 0; i < CONGRATS_LINES; ++i) l2_print(32, 16 + i * 12, congrats[i], 6, 0);
    }
}
