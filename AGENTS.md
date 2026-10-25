# AGENTS.md

Notes for AI coding agents working on RobboNext, a conversion of the 1989
Atari 8-bit game Robbo to the ZX Spectrum Next. `README.md` describes the
game, the screen layout and the asset pipeline; this file covers what is
easy to break.

## Build and check

- `make` builds `robbo.nex` (z88dk 2.4, SDCC, `-clib=sdcc_iy -startup=31`).
  Pass `Z88DK=/path/to/z88dk` if it is not in `/Applications/z88dk`.
- `make assets` regenerates `res/*.bin` and `src/gen_assets.h` from
  `art/hd/*.png` and `art/splash/` (Python 3 + Pillow). `make` does it by
  itself when the art changes.
- After a change: the build must finish without new warnings, and the game
  must be checked in an emulator (CSpect, or ZEsarUX with
  `--totalaychips 3`). Test the splash, title, a level with scrolling, losing
  a life (hold `R`), `BREAK` back to the title, and the ending (cheat mode `C`
  on the title, then `N` to skip planets).
- `robbo.nex` is committed: rebuild it whenever code or assets change.

## Generated files

- Do not edit `src/gen_assets.h` or the converted graphics in `res/`
  (`tiles.bin`, `spr_*.bin`, `digits.bin`, `title_*`, `ending_*`, `splash_*`,
  `tm_font.bin`, `ramps.bin`). Change `art/hd/*.png` or
  `scripts/convert_assets.py` and run `make assets`.
- `res/levels.bin`, `res/sound.bin`, `res/flags.bin` and `res/font.bin` are
  source data, not generated.

## Memory (the usual source of bugs)

- 8K pages; numbers in `src/hw.h` must match the `SECTION PAGE_nn_...` names
  in `src/assets.asm`. Pages 87-95 are free.
- Page 46 is mapped at `0x0000` for the whole game instead of the ROM: font,
  colour ramps, sound data, and the constants of `data.c` (compiled with
  `--constseg=PAGE_46_RESIDENT_C`, at `0x1000`, about 1.5K free).
- Page 47 is mapped at `0x6000`: `title.c` and `fx.c` are compiled into it
  (`--codeseg/--constseg=PAGE_47_CODE`, about 3.5K free). A new file that
  should live there needs its own rule in the `makefile`.
- Main code and variables run from `0x8000`; about 10K is left below the
  stack at `0xFFFE`. Check `robbo.map` (`__BSS_END_tail`) after adding data.
- `page_in()` maps a page at `0x2000` (`PAGE_WINDOW`); always `page_out()`
  afterwards. Tile graphics are mapped at `0x4000` over bank 5 only inside
  `render_board()`, and bank 5 must be restored (the tilemap lives there).
- The ROM is never used and interrupts are disabled; frame timing polls the
  video line (`hw_wait_frame()`).

## Game rules and timing

- `game.c` keeps the rules of the original: do not change how objects,
  creatures or scoring behave unless asked. Map tiles keep their original
  numbers (see the `T_*` constants and `res/flags.bin`).
- The game logic does not touch the screen. `main.c` draws the previous
  update first (while the beam is below the board), then updates: 3 updates
  every 5 frames at 50 Hz, 1 every 2 frames at 60 Hz. Keep drawing within
  that budget; the game must not drop frames on any level.
- Write to the map with `mset()` (or bump `map_version` yourself), or the
  board will not be redrawn.
- Effects in `fx.c` are visual only and use their own random numbers, so the
  game plays the same with or without them.

## Graphics

- Layer 2 is 320x256, stored column by column; the board is a ring of 16 map
  rows scrolled with the layer 2 Y offset.
- Palette index = colour * 8 + shade (shade 3 is the colour itself); index 0
  is transparent and never used as a colour. Colours 0-15 are the base
  colours (some are swapped per level), 16-31 extra colours.
- Sprites are 8-bit, one pattern set per screen (`sprite_load_set`); slots
  are assigned in `render.c` (robbo 0, particles 32-63, status bar 90-121).

## Style

- C for SDCC: no dynamic allocation, no floating point, types from
  `src/types.h` (`u8`, `s16`, ...). Prefer `u8` loop counters.
- Short English comments that explain why; keep the comment density and
  naming of the surrounding code.
- Hardware access goes through `src/hw.c`.

## Licences

- Code: GPL-3.0 (`LICENSE`). Levels, sound and graphics derived from
  bocianu's version: CC BY-NC-SA 4.0. Robbo is (C) 1989 LK Avalon and
  Janusz Pelc; this is a non-commercial fan conversion.
- Do not add third-party code, graphics or music without naming the author
  and licence in `README.md`.
