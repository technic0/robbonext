///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - sfx and music player on the Next's three AY chips
//
//  AY#1 (and channel A of AY#3) play music, AY#2 plays sound effects.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _sound_h_
#define _sound_h_

#include "types.h"

void sound_init(bool hz60);
void sound_update(void);        // call once per video frame

void sfx(u8 n);
void music_play(u8 pattern);
void music_stop(void);
bool music_playing(void);

// sfx numbers used by the game (entries in res/sound.bin)
#define SFX_STEP        0
#define SFX_LANDING     1
#define SFX_AMMO        2
#define SFX_PUSH        3
#define SFX_PISTOL      4
#define SFX_SCREW       5
#define SFX_EXPLOSION   6
#define SFX_PUFF        7
#define SFX_KEY         8
#define SFX_DOOR        9
#define SFX_TELEPORT    10
#define SFX_EXIT        11
#define SFX_LAST_SCREW  12
#define SFX_EXTRA_LIFE  13

#endif
