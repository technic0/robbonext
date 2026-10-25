///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - hardware layer for the ZX Spectrum Next
//
///////////////////////////////////////////////////////////////////////////////

#include <arch/zxn.h>
#include <intrinsic.h>
#include <string.h>
#include <z80.h>

#include "hw.h"
#include "gen_assets.h"

extern u8 ramp_data[];     // 8 shades of 48 colours, 9-bit (resident page)

///////////////////////////////////////////////////////////////////////////////
// next registers and paging

void nextreg(u8 reg, u8 val)
{
    IO_NEXTREG_REG = reg;
    IO_NEXTREG_DAT = val;
}

u8 nextreg_read(u8 reg)
{
    IO_NEXTREG_REG = reg;
    return IO_NEXTREG_DAT;
}

void page_in(u8 page)
{
    nextreg(0x51, page);
}

void page_out(void)
{
    nextreg(0x51, 0xFF);
}

void layers_init(void)
{
    // S U L, sprites visible, over the border and clipped there too
    nextreg(0x15, (2 << 2) | 0x23);
}

///////////////////////////////////////////////////////////////////////////////
// palettes

void pal_set_rgb9(u8 which, u8 index, u16 rgb9)
{
    nextreg(0x43, which);
    nextreg(0x40, index);
    nextreg(0x44, (u8)(rgb9 >> 1));
    nextreg(0x44, (u8)(rgb9 & 1));
}

// ramps are stored base 0-15, secret 128-143, extra 16-31
static const u8* ramp_of(u8 colour)
{
    u8 r = colour;
    if (colour & 0x80) r = 16 + (colour & 15);
    else if (colour >= 16) r = 32 + (colour - 16);
    return ramp_data + r * 16;
}

u16 ramp_rgb9(u8 colour, u8 shade)
{
    const u8* p = ramp_of(colour) + shade * 2;
    return ((u16)p[0] << 1) | p[1];
}

void pal_ramp(u8 which, u8 slot, u8 colour)
{
    const u8* shades = ramp_of(colour);
    u8 i;
    nextreg(0x43, which);
    nextreg(0x40, slot * 8);
    for (i = 0; i < 16; ++i) nextreg(0x44, shades[i]);
}

void pal_default(u8 which)
{
    u8 i;
    for (i = 0; i < 32; ++i) pal_ramp(which, i, i);
}

void pal_load(u8 which, u16 offset)
{
    const u8* p = PAGE_WINDOW + offset;
    u16 n = 512;
    page_in(PAGE_MISC);
    nextreg(0x43, which);
    nextreg(0x40, 0);
    while (n--) nextreg(0x44, *p++);
    page_out();
}

///////////////////////////////////////////////////////////////////////////////
// layer 2

void layer2_enable(bool on)
{
    nextreg(0x69, on ? 0x80 : 0x00);
}

void layer2_clip(u16 x1, u16 x2, u8 y1, u8 y2)
{
    nextreg(0x1C, 0x01);
    nextreg(0x18, x1 >> 1);
    nextreg(0x18, x2 >> 1);
    nextreg(0x18, y1);
    nextreg(0x18, y2);
}

void layer2_scroll_y(u8 y)
{
    nextreg(0x17, y);
}

void layer2_picture(u8 page)
{
    u8 i;
    // source at 0x2000, layer 2 page at 0x4000 (over bank 5 for a moment)
    for (i = 0; i < 10; ++i) {
        page_in(page + i);
        nextreg(0x52, PAGE_LAYER2 + i);
        memcpy((void*)0x4000, PAGE_WINDOW, 8192);
    }
    nextreg(0x52, PAGE_BANK5);
    page_out();
}

///////////////////////////////////////////////////////////////////////////////
// tilemap

void tilemap_enable(bool on)
{
    // 40x32 with attribute bytes, 256 patterns
    nextreg(0x6B, on ? 0x80 : 0x00);
}

void tilemap_clip(u16 x1, u16 x2, u8 y1, u8 y2)
{
    nextreg(0x1C, 0x08);
    nextreg(0x1B, x1 >> 1);
    nextreg(0x1B, x2 >> 1);
    nextreg(0x1B, y1);
    nextreg(0x1B, y2);
}

void tilemap_scroll_y(u8 y)
{
    nextreg(0x31, y);
}

void tilemap_clear(void)
{
    memset(TILEMAP, 0, TILEMAP_W * 32 * 2);
}

void tilemap_patterns(u16 offset, u16 bytes)
{
    page_in(PAGE_MISC);
    memcpy(TILE_DEFS, PAGE_WINDOW + offset, bytes);
    page_out();
}

///////////////////////////////////////////////////////////////////////////////
// sprites

void sprite_show(u8 slot, s16 x, s16 y, u8 pat, u8 mirror, u8 scale)
{
    IO_SPRITE_SLOT = slot;
    IO_SPRITE_ATTRIBUTE = (u8)x;
    IO_SPRITE_ATTRIBUTE = (u8)y;
    IO_SPRITE_ATTRIBUTE = mirror | ((x >> 8) & 1);
    IO_SPRITE_ATTRIBUTE = 0xC0 | pat;
    IO_SPRITE_ATTRIBUTE = scale | ((y >> 8) & 1);
}

void sprite_show_big(u8 slot, s16 x, s16 y, u8 pat, u8 w, u8 h)
{
    u8 i, j;
    for (j = 0; j < h; ++j)
        for (i = 0; i < w; ++i)
            sprite_show(slot++, x + i * 16, y + j * 16, pat++, 0, 0);
}

void sprite_hide(u8 slot)
{
    // keep bit 6 of the 4th byte set (5 byte attributes), otherwise the
    // 5th write would land in the next slot's first byte
    IO_SPRITE_SLOT = slot;
    IO_SPRITE_ATTRIBUTE = 0;
    IO_SPRITE_ATTRIBUTE = 0;
    IO_SPRITE_ATTRIBUTE = 0;
    IO_SPRITE_ATTRIBUTE = 0x40;
    IO_SPRITE_ATTRIBUTE = 0;
}

void sprite_hide_all(void)
{
    u8 i;
    for (i = 0; i < 128; ++i) sprite_hide(i);
}

void sprite_clip(u16 x1, u16 x2, u8 y1, u8 y2)
{
    // over the border the x coordinates are halved, as for layer 2
    nextreg(0x1C, 0x02);
    nextreg(0x19, x1 >> 1);
    nextreg(0x19, x2 >> 1);
    nextreg(0x19, y1);
    nextreg(0x19, y2);
}

static const u8 set_pages[3] = { PAGE_SPR_GAME, PAGE_SPR_TITLE, PAGE_SPR_ENDING };
static const u8 set_patterns[3] = { SPRITES_GAME, SPRITES_TITLE, SPRITES_ENDING };

void sprite_load_set(u8 set)
{
    const u8* src = PAGE_WINDOW;
    u16 n = set_patterns[set] * 256;
    page_in(set_pages[set]);
    IO_SPRITE_SLOT = 0;
    while (n--) IO_SPRITE_PATTERN = *src++;
    page_out();
}

void sprite_pattern(u8 pat, const u8* data)
{
    u16 n = 256;
    IO_SPRITE_SLOT = pat;
    while (n--) IO_SPRITE_PATTERN = *data++;
}

///////////////////////////////////////////////////////////////////////////////
// init and frame timing

void hw_init(void)
{
    intrinsic_di();
    nextreg(0x50, PAGE_RESIDENT);                   // replaces the ROM
    nextreg(0x53, PAGE_CODE);                       // more code at 0x6000

    nextreg(0x07, 0x03);                            // 28 MHz
    nextreg(0x08, nextreg_read(0x08) | 0x42);       // no contention, turbosound
    layers_init();
    nextreg(0x4A, 0x00);                            // fallback colour black
    nextreg(0x4B, 0x00);                            // sprite transparent index
    nextreg(0x4C, 0x00);                            // tilemap transparent index
    nextreg(0x68, 0x80);                            // ULA off
    nextreg(0x12, PAGE_LAYER2 / 2);                 // layer 2 in banks 8-12
    nextreg(0x70, 0x10);                            // layer 2 320x256x8
    nextreg(0x16, 0);                               // layer 2 scroll
    nextreg(0x71, 0);
    nextreg(0x17, 0);
    nextreg(0x6C, 0x00);                            // default tile attribute
    nextreg(0x6E, 0x00);                            // tilemap at 0x4000
    nextreg(0x6F, 0x0A);                            // patterns at 0x4A00
    nextreg(0x2F, 0);                               // tilemap scroll
    nextreg(0x30, 0);
    nextreg(0x31, 0);
    IO_FE = 0;                                      // black border

    layer2_enable(false);
    tilemap_enable(false);
    tilemap_clear();
    sprite_hide_all();
    layer2_clip(0, 319, 0, 255);
    tilemap_clip(0, 319, 0, 255);
    sprite_clip(0, 319, 0, 255);
    pal_default(PAL_LAYER2);
    pal_default(PAL_SPRITES);
}

// active video line, 9 bits
static u16 video_line(void)
{
    u8 hi = nextreg_read(0x1E) & 1;
    return ((u16)hi << 8) | nextreg_read(0x1F);
}

// wait until the video line becomes 'line' (if it is 'line' now, for the
// next time round)
static void hw_wait_line(u16 line)
{
    while (video_line() == line) ;
    while (video_line() != line) ;
}

void hw_wait_frame(void)
{
    // the board ends at line 199 of the 192 line display (y 231 of the
    // 320x256 area): drawing starts there and has until the board's top
    // line comes round again
    hw_wait_line(200);
}

bool hw_is_60hz(void)
{
    return (nextreg_read(0x05) & 0x04) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// input

u8 btn_state;
u8 btn_pressed;

bool key_down(u16 scancode)
{
    return (z80_inp((scancode & 0xFF00) | 0xFE) & (u8)scancode) == 0;
}

bool any_key(void)
{
    u8 joy = IO_1F;
    // reading port 0xFE with all half rows selected
    if ((z80_inp(0x00FE) & 0x1F) != 0x1F) return true;
    return joy != 0xFF && (joy & 0x10);
}

void input_update(void)
{
    u8 b = 0;
    u8 joy;

    // QAOP, cursor keys (5-8, also cursor joystick) and Kempston
    if (key_down(0xDF02) || key_down(0xF710)) b |= BTN_LEFT;    // O, 5
    if (key_down(0xDF01) || key_down(0xEF04)) b |= BTN_RIGHT;   // P, 8
    if (key_down(0xFB01) || key_down(0xEF08)) b |= BTN_UP;      // Q, 7
    if (key_down(0xFD01) || key_down(0xEF10)) b |= BTN_DOWN;    // A, 6
    if (key_down(KEY_SPACE) || key_down(0x7F04) ||              // space, M
        key_down(0xEF01) || key_down(KEY_ENTER)) b |= BTN_FIRE; // 0, enter

    joy = IO_1F;
    if (joy != 0xFF) {
        if (joy & 0x01) b |= BTN_RIGHT;
        if (joy & 0x02) b |= BTN_LEFT;
        if (joy & 0x04) b |= BTN_DOWN;
        if (joy & 0x08) b |= BTN_UP;
        if (joy & 0x10) b |= BTN_FIRE;
    }

    btn_pressed = b & ~btn_state;
    btn_state = b;
}
