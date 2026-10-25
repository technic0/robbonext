///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - hardware layer for the ZX Spectrum Next
//
//  Next registers, memory paging, palettes, layer 2, tilemap, sprites and
//  input. Everything that touches the machine directly lives here.
//
//  The screen is the full 320x256 area: layer 2 in 320x256 mode, the
//  tilemap (40x32, used for scrolling text) and sprites over the
//  border. The ULA is switched off.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _hw_h_
#define _hw_h_

#include "types.h"

///////////////////////////////////////////////////////////////////////////////
// memory layout

// 8K pages holding the assets (see assets.asm)
#define PAGE_LEVELS             42      // to 45
#define PAGE_RESIDENT           46      // font, sound, ramps, tables, at 0x0000
#define PAGE_CODE               47      // code, always at 0x6000
#define PAGE_TILES              48      // to 52, 32 tiles per page
#define PAGE_SPR_GAME           53
#define PAGE_SPR_TITLE          54
#define PAGE_SPR_ENDING         55
#define PAGE_MISC               56      // digits, tilemap font
#define PAGE_TITLE_PIC          57      // to 66
#define PAGE_ENDING_PIC         67      // to 76
#define PAGE_SPLASH_PIC         77      // to 86

// offsets in PAGE_MISC
#define MISC_DIGITS             0
#define MISC_TM_FONT            1280
#define MISC_SPLASH_PAL         4352    // 256 colours, 9-bit

// layer 2 (320x256, 8 bits per pixel, column by column) in pages 16-25
#define PAGE_LAYER2             16

// bank 5 (page 10) at 0x4000: tilemap and its patterns
#define PAGE_BANK5      10
#define TILEMAP         ((u8*)0x4000)   // 40x32 entries of 2 bytes
#define TILEMAP_W       40
#define TILE_DEFS       ((u8*)0x4A00)   // 4-bit patterns, 32 bytes each

// map an 8K page at PAGE_WINDOW (0x2000-0x3FFF), or put the ROM back
#define PAGE_WINDOW     ((const u8*)0x2000)
void page_in(u8 page);
void page_out(void);

///////////////////////////////////////////////////////////////////////////////
// next registers

void nextreg(u8 reg, u8 val);
u8   nextreg_read(u8 reg);

void hw_init(void);
void hw_wait_frame(void);
bool hw_is_60hz(void);

// layer order: sprites on top, then the tilemap (text) over layer 2
void layers_init(void);

///////////////////////////////////////////////////////////////////////////////
// palettes

#define PAL_LAYER2    0x10
#define PAL_SPRITES   0x20
#define PAL_TILEMAP   0x30

void pal_set_rgb9(u8 which, u8 index, u16 rgb9);

// Layer 2 and sprite pixels are colour * 8 + shade; this loads the 8 shades
// of a colour (0-15 base, 16-31 extra, 128-143 secret) into the entries
// of colour slot 'slot'
void pal_ramp(u8 which, u8 slot, u8 colour);
// one shade of a colour, as 9-bit RGB
u16  ramp_rgb9(u8 colour, u8 shade);
// default ramps for all 32 colour slots
void pal_default(u8 which);
// 256 colours (512 bytes, 9-bit) stored in PAGE_MISC at 'offset'
void pal_load(u8 which, u16 offset);

///////////////////////////////////////////////////////////////////////////////
// layer 2

void layer2_enable(bool on);
// clip window in 320x256 pixels (x is stored halved by the hardware)
void layer2_clip(u16 x1, u16 x2, u8 y1, u8 y2);
void layer2_scroll_y(u8 y);
// copy a 320x256 picture stored from 'page' on into layer 2
void layer2_picture(u8 page);

// copy a 16x16 tile: src in a page mapped at 0x4000, dst at 0x2000
void blit_tile(const u8* src, u8* dst);

///////////////////////////////////////////////////////////////////////////////
// tilemap

void tilemap_enable(bool on);
void tilemap_clip(u16 x1, u16 x2, u8 y1, u8 y2);
void tilemap_scroll_y(u8 y);
void tilemap_clear(void);
// copy 4-bit patterns from PAGE_MISC to the pattern area
void tilemap_patterns(u16 offset, u16 bytes);

///////////////////////////////////////////////////////////////////////////////
// sprites

#define SPR_MIRROR_X  0x08
#define SPR_MIRROR_Y  0x04

#define SCALE_X1 0x00
#define SCALE_X2 0x08
#define SCALE_Y1 0x00
#define SCALE_Y2 0x02

// 8-bit patterns; index 0 is transparent
// x,y in pixels of the 320x256 area
void sprite_show(u8 slot, s16 x, s16 y, u8 pat, u8 mirror, u8 scale);
// a sprite made of w x h patterns of 16x16, numbered row by row
void sprite_show_big(u8 slot, s16 x, s16 y, u8 pat, u8 w, u8 h);
void sprite_hide(u8 slot);
void sprite_hide_all(void);
void sprite_clip(u16 x1, u16 x2, u8 y1, u8 y2);

// pattern sets: one per screen
#define SET_GAME    0
#define SET_TITLE   1
#define SET_ENDING  2
void sprite_load_set(u8 set);
// replace one pattern with 256 bytes
void sprite_pattern(u8 pat, const u8* data);

///////////////////////////////////////////////////////////////////////////////
// input

#define BTN_LEFT   0x01
#define BTN_RIGHT  0x02
#define BTN_UP     0x04
#define BTN_DOWN   0x08
#define BTN_FIRE   0x10

extern u8 btn_state;     // buttons held this frame
extern u8 btn_pressed;   // buttons that went down this frame

void input_update(void);
bool key_down(u16 scancode);
// any key of the keyboard, or joystick fire
bool any_key(void);

// keyboard scancodes: high byte = half row port, low byte = bit mask
#define KEY_CAPS   0xFE01
#define KEY_SPACE  0x7F01
#define KEY_ENTER  0xBF01
#define KEY_A 0xFD01
#define KEY_B 0x7F10
#define KEY_C 0xFE08
#define KEY_J 0xBF08
#define KEY_K 0xBF04
#define KEY_L 0xBF02
#define KEY_N 0x7F08
#define KEY_R 0xFB08
#define KEY_S 0xFD02
#define KEY_W 0xFB02

#endif
