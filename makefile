#
# Makefile for Robbo project on ZX Spectrum Next
#

# Output application name
NEX_APP_NAME = robbo

# z88dk install location (override with: make Z88DK=/path/to/z88dk)
Z88DK ?= /Applications/z88dk

# CSpect install location
CSPECT ?= /Applications/CSpect3_0_2_1

# Python script for asset conversion (needs Pillow)
PYTHON ?= python3
CONVERT_ASSETS = $(PYTHON) scripts/convert_assets.py

# Source cart, redrawn art and generated files
CART = art/cart/robbo.p8.png
HD_ART = art/hd/tiles_hd.png art/hd/sprites_hd.png art/hd/title.png art/hd/ending.png \
         art/splash/robbo_320x256.sl2 art/splash/robbo_320x256.npl
ASSETS = res/tiles.bin res/spr_game.bin res/spr_title.bin res/spr_ending.bin \
         res/digits.bin res/title_a.bin res/title_b.bin res/ending_a.bin res/ending_b.bin \
         res/splash_a.bin res/splash_b.bin res/splash.npl \
         res/tm_font.bin \
         res/ramps.bin res/levels.bin res/sound.bin src/gen_assets.h

# Compiler
ZCC = ZCCCFG=$(Z88DK)/lib/config PATH=$(Z88DK)/bin:$$PATH $(Z88DK)/bin/zcc

# Environment variables for z88dk
export ZCCCFG=$(Z88DK)/lib/config
export PATH := $(Z88DK)/bin:$(PATH)

# Compilation flags. -pragma-include applies to EVERY compiled file.
CFLAGS = +zxn -subtype=nex -vn -O3 -compiler=sdcc -clib=sdcc_iy -startup=31 --list -create-app -pragma-include:src/zpragma.inc

# constant tables and texts live in the resident page at 0x0000 (see assets.asm)
CFLAGS_RESIDENT = --constseg=PAGE_46_RESIDENT_C

# code that runs from page 47, mapped at 0x6000 for the whole game
CFLAGS_PAGE47 = --codeseg=PAGE_47_CODE --constseg=PAGE_47_CODE

# List of object files (.o) to compile and link.
OBJS = src/main.o \
       src/game.o \
       src/render.o \
       src/title.o \
       src/fx.o \
       src/sound.o \
       src/data.o \
       src/hw.o \
       src/assets.o \
       src/blit.o

HEADERS = $(wildcard src/*.h)

#
# Main rule building the entire project
#
all: $(NEX_APP_NAME).nex

#
# Asset conversion rule - depends on the original cart and the script
#
$(ASSETS): $(CART) $(HD_ART) scripts/convert_assets.py
	$(CONVERT_ASSETS)

#
# Linking rule: links all .o files into one .nex file
#
$(NEX_APP_NAME).nex: $(OBJS)
	$(ZCC) $(CFLAGS) -m -o $(NEX_APP_NAME) $(OBJS)

#
# Compilation rules: how to create .o files from .c and .asm files
# $< means source file (e.g. main.c), $@ means target file (e.g. main.o)
#

src/%.o: src/%.c $(HEADERS) src/gen_assets.h
	$(ZCC) $(CFLAGS) -c $< -o $@

src/data.o: src/data.c $(HEADERS) src/gen_assets.h
	$(ZCC) $(CFLAGS) $(CFLAGS_RESIDENT) -c $< -o $@

src/title.o: src/title.c $(HEADERS) src/gen_assets.h
	$(ZCC) $(CFLAGS) $(CFLAGS_PAGE47) -c $< -o $@

src/fx.o: src/fx.c $(HEADERS) src/gen_assets.h
	$(ZCC) $(CFLAGS) $(CFLAGS_PAGE47) -c $< -o $@

src/assets.o: src/assets.asm $(ASSETS) res/font.bin
	$(ZCC) $(CFLAGS) -c $< -o $@

src/blit.o: src/blit.asm
	$(ZCC) $(CFLAGS) -c $< -o $@

#
# Rule for running CSpect emulator
#
run: $(NEX_APP_NAME).nex
	@echo "Launching CSpect emulator..."
	@echo "If CSpect crashes, try 'make run-safe' instead"
	cd "$(CSPECT)" && MONO_WINFORMS_XIM_STYLE=disabled mono CSpect.exe "$(shell pwd)/$(NEX_APP_NAME).nex"

#
# Alternative safer run method (may work better on some systems)
#
run-safe: $(NEX_APP_NAME).nex
	@echo "Launching CSpect with compatibility settings..."
	cd "$(CSPECT)" && MONO_WINFORMS_XIM_STYLE=disabled MONO_THEME=clearlooks mono CSpect.exe "$(shell pwd)/$(NEX_APP_NAME).nex"

#
# Manual launch helper - shows the command to run CSpect manually
#
run-manual: $(NEX_APP_NAME).nex
	@echo "=================================================================="
	@echo "To run CSpect manually, use this command:"
	@echo 'cd "$(CSPECT)" && mono CSpect.exe "$(shell pwd)/$(NEX_APP_NAME).nex"'
	@echo "=================================================================="

#
# Rule for building and running in one step
#
test: all run

#
# Rule for cleaning the project (generated assets are kept, see 'assets')
#
clean:
	rm -f src/*.o $(NEX_APP_NAME).nex *.lis ./src/*.lis $(NEX_APP_NAME).map $(NEX_APP_NAME)_*.bin

#
# Force asset regeneration
#
assets:
	$(CONVERT_ASSETS)

.PHONY: all clean run run-safe run-manual test assets
