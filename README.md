# RobboNext

A conversion of **Robbo**, the 1989 Atari 8-bit classic by Janusz Pelc (LK Avalon), to the **ZX Spectrum Next** computer (including clones like ZX Spectrum N-GO, ZXNext_MISTer, etc.).

## About The Project

> The goal of this project is to create a fully playable version of the cult classic game "Robbo", keeping the planets, rules and look & feel of the 1989 Atari original. The project leverages the advanced hardware capabilities of the ZX Spectrum Next computer, such as the sprite layer and the 256 colour Layer 2, to achieve high performance and smooth animation.
>
> The project's architecture is heavily inspired by and based on the reference code from the game **[CelesteNext](https://github.com/dcrespo3d/CelesteNext)** by David Crespo.

### Current Status

**Complete and playable.**

The whole game is there:

* all 56 planets of the Atari original,
* every object and creature: screws, keys, doors, ammo, bombs and chain explosions, crates and loose crates, magnets, teleports, question marks, guns, rotating and moving guns, lasers, blasters, barriers, bats, creatures and eyes,
* smooth vertical scrolling, the curtain transitions, the flashing background after the last screw and the blinking capsule,
* the title screen with the colour-cycling logo, scrolling instructions and title music,
* the ending sequence,
* all sound effects and the music, played on the Next's three AY chips,
* cheat mode,
* **HD graphics**: every tile and sprite redrawn at 16x16 with 256 colours, shading,
  soft shadows and glowing energy objects, keeping the colours and shapes of the original.
* **Full 320x256 screen**: 14 rows of the planet are visible at once,
  with a graphical status bar below the board.
* **Particles**: debris, sparks and smoke from explosions, stars on pickups and teleports.
* **More animation**: screws glint, laser heads pulse, teleports swirl, capsule lights
  chase, barriers flow and the void twinkles.
* **Full-screen title and ending pictures**, with smoothly scrolling instructions.
* **Opening picture** (`art/splash`, 320x256 with its own 256 colour palette)
  before the title screen; any key or joystick fire goes on to the title.

## Technology

* **Target Platform:** ZX Spectrum Next
* **Language:** C and Z80 Assembly
* **Toolchain:** `z88dk` with the `sdcc` compiler and `sdcc_iy` library
* **Automation:** `make`

## Controls

| Action | Keys |
| --- | --- |
| Move | `Q` `A` `O` `P`, cursor keys, Kempston joystick |
| Shoot | hold fire and press a direction |
| Fire | `SPACE`, `M`, `0`, `ENTER` or joystick fire |
| Give up (lose a life) | hold `R` |
| Back to the title | `BREAK` |
| Toggle cheat mode | `C` on the title screen |

Joysticks work in Kempston or Cursor mode.

As on the Atari, typing `janusz pelc` on the title screen starts the next
game with 80 lives (a sound confirms the code; the space in it does not
start the game, and its C does not toggle cheat mode).

In cheat mode scores are not counted and these keys work during the game:
`N` next level, `B` previous level, `J` ammo, `S` screw, `K` key, `L` life, `W` walk through walls.

## Getting Started

The following instructions are for macOS but can be easily adapted for Linux.

### 1. Prerequisites

Before you begin, ensure you have the following tools installed:

* **Homebrew** (package manager for macOS)
* **z88dk:** The latest stable binary release (2.4 is known to work). Recommended installation path is `/Applications/z88dk`.
* **Python 3** and the **Pillow** library (only needed to regenerate the assets in `res/`):
    ```bash
    pip3 install Pillow
    ```
* **Mono** (to run the emulator)
* **CSpect** (ZX Spectrum Next emulator)

### 2. Environment Setup

1.  Add the following lines to the end of your shell's configuration file (e.g., `~/.zshrc` for Zsh or `~/.bash_profile` for Bash):
    ```bash
    export Z88DK_HOME="/Applications/z88dk"
    export PATH="$Z88DK_HOME/bin:$PATH"
    ```
2.  Restart your terminal or run the command `source ~/.zshrc` to apply the changes.

### 3. Compilation

Once the environment is configured, compiling the project is very simple:

1.  Clone the project repository.
2.  Navigate to the project's root folder in your terminal.
3.  Execute the command:
    ```bash
    make
    ```
    This will compile all source files and link them into a single executable file: `robbo.nex`.

    If z88dk or CSpect live somewhere else, pass their location:
    ```bash
    make Z88DK=/path/to/z88dk CSPECT=/path/to/CSpect
    ```

4.  To clean the project of intermediate files, run:
    ```bash
    make clean
    ```

5.  To regenerate the converted graphics from the HD art, run:
    ```bash
    make assets
    ```
    `make` does this by itself when `art/hd/*.png` or `art/splash` change.

## Project Structure

```bash
robbonext/
├── art/             # Source graphics (.png)
│   ├── hd/          # HD graphics: tiles, sprites, title and ending (indexed, editable)
│   └── splash/      # opening picture: layer 2 data (.sl2) and palette (.npl)
├── res/             # Binary assets: levels, sound, tile flags and the converted graphics
├── scripts/         # convert_assets.py: HD art -> res/*.bin, src/gen_assets.h
├── src/             # All source code files (.c, .h, .asm)
└── makefile         # The file for automating the build process
```

| File | Contents |
| --- | --- |
| `src/main.c` | Main loop and frame timing |
| `src/game.c` | Game logic |
| `src/render.c` | Game screen: board on layer 2, robbo, curtain, status bar |
| `src/fx.c` | Particles |
| `src/title.c` | Title screen and ending |
| `src/sound.c` | Sound effects and music player for the AY chips |
| `src/hw.c` | Next registers, paging, palettes, layer 2, tilemap, sprites, input |
| `src/blit.asm` | Copies a 16x16 tile into layer 2 |
| `src/data.c` | Level colours, texts and other tables |
| `src/assets.asm` | Binary assets placed in memory pages |

You can also compile and run CSpect (ZX Spectrum Next Emulator) to test the game by running:
```bash
make test
```
This step assumes you have CSpect emulator installed on your local machine.

## How It Works

### Assets

The 56 levels (`res/levels.bin`, 512 bytes each), the sound effects and music
(`res/sound.bin`) and the tile flags (`res/flags.bin`) are kept as binary data.
`scripts/convert_assets.py` converts the graphics in `art/hd/` and `art/splash/`
into `res/*.bin` and writes the tile and sprite tables in `src/gen_assets.h`.

### HD graphics

The HD art is stored as indexed PNGs that can be edited in any pixel art
editor that keeps the palette (Aseprite, GrafX2, Pro Motion...):

* `tiles_hd.png` holds the board tiles: tile `n` is
  at column `n % 16`, row `n / 16`, each 16x16. Below them are the extra
  animation frames (`ANIM` in `convert_assets.py`).
* `sprites_hd.png` holds robbo, the status bar icons, panel and digits, the
  particles, the title logo and the ending scene; `SPRITE_LAYOUT` in
  `convert_assets.py` lists where each one is.
* `title.png` and `ending.png` are the 320x256 pictures of those screens.

A pixel's palette index is **colour * 8 + shade**. Shade 3 is the colour
itself, 0-2 are darker and 4-7 lighter. Colours 0-15 are the base colours;
16-31 are extra colours of the HD art (gold, chrome, plasma, fire, ...).
Index 0 is transparent and never used as a colour. The PNG palette only
previews the default colours: in the game every level builds the shades of
its own background, wall and crate colours, just like the planets of the
Atari original change colour, so one drawing serves all 56 planets.

### Screen

The game uses the whole 320x256 area; the ULA is switched off.

| Layer | Used for |
| --- | --- |
| Layer 2 (320x256, 256 colours) | the board, the title and ending pictures |
| Sprites (8-bit, 16x16) | robbo, status bar, particles, title logo, ending scene |
| Tilemap | the scrolling instructions on the title screen |

Layer 2 holds a ring of 16 map rows (256 lines) of which 14 are shown. A
cell is copied from the tile graphics only when what it shows changes, and
nothing is scanned at all when the map, the camera and the animation phase
did not change. Vertical scrolling uses the layer 2 scroll register; the
curtain is the layer 2 clip window.

The status bar is a row of sprites. Its numbers are drawn into sprite
patterns when they change.

### Timing

The game logic updates 30 times per second. The Next runs one update every
other frame at 60 Hz, or three updates every five frames at 50 Hz. Each update
frame first draws the previous update's result as soon as the beam has left
the board, then runs the game logic. The sound player runs every frame.

### Memory

| Page | Contents |
| --- | --- |
| 10 | tilemap and its patterns (bank 5) |
| 16-25 | layer 2 |
| 42-45 | levels, 16 per page |
| 46 | font, colour shades, sound data, tables and texts; mapped at `0x0000` instead of the ROM |
| 47 | code: title, ending, particles; mapped at `0x6000` |
| 48-52 | tiles, 32 per page |
| 53-55 | sprite patterns of each screen |
| 56 | status bar digits, tilemap font |
| 57-66, 67-76 | title and ending pictures |
| 77-86 | opening picture (its palette is in page 56) |

The rest of the code and the variables live at `0x8000`-`0xD6xx`. The ROM is
not used, and interrupts are disabled. Pages are mapped temporarily at
`0x2000` (and tile graphics at `0x4000`) to draw and to load data.

### Notes on the conversion

* Robbo cannot be killed again once he is dead or leaving in his capsule
  (two explosions in one frame no longer cost two lives).
* A barrier no longer tries to remove a creature using stale coordinates
  (it never removed anything in practice).
* Laser beams and blasts stop at the edge of the map.
* All waveforms are AY square waves except noise.
* HD graphics.
* 14 rows of the planet are visible at once; the camera keeps robbo two rows
  away from the edges of the screen.

## Acknowledgements

* **Janusz Pelc and LK Avalon:** For creating the original, unforgettable "Robbo" for the Atari 8-bit computers (1989).
* **bocianu:** For bringing the Atari original to PICO-8 with its levels and look & feel intact; his version was the inspiration for this conversion.
* **David Crespo ([@dcrespo3d](https://github.com/dcrespo3d)):** For creating the fantastic game `CelesteNext` and sharing its code. The architecture of `RobboNext` is largely based on the solutions and modules from that project.

## License

ROBBO is (C) 1989 LK Avalon and Janusz Pelc. This ZX Spectrum Next version
(TecHniC, 2026) is a non-commercial fan conversion made with the help of AI;
all rights to the game belong to their owners.

The code of this project is distributed under the **GNU General Public License v3.0**. See the `LICENSE` file for more details.

The levels, sound and tile data in `res/` come from bocianu's version, published under **CC BY-NC-SA 4.0** on the [Lexaloffle BBS](https://www.lexaloffle.com/bbs/?tid=52076). The graphics in `art/hd/` are derived from his and shared under the same licence.

The opening picture in `art/splash/` was generated with Google Gemini.
