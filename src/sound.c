///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - sfx and music player on the Next's three AY chips
//
//  The sfx and music data (res/sound.bin) are played as they are stored.
//  Every waveform becomes an AY square wave except noise, which uses the
//  AY noise generator. Slides, drops, fades, vibrato and arpeggios are
//  approximated per video frame.
//
///////////////////////////////////////////////////////////////////////////////

#include <arch/zxn.h>

#include "sound.h"

extern u8 sound_data[];     // 256 bytes of music patterns, then 68 per sfx

#define MUSIC           (sound_data)
#define SFX_DATA(n)     (sound_data + 256 + (u16)(n) * 68)
#define SFX_COUNT       26
#define NOTE_SAMPLES    183     // note length unit at 22050 Hz

#define CHANNELS        7       // 0-3 music, 4-6 sfx
#define FIRST_SFX_CHAN  4

typedef struct
{
    const u8* data;     // NULL when idle
    u8  id;
    u8  note;
    u8  len;
    u8  loop_start;
    u8  loop_end;       // loops when loop_start < loop_end
    u8  pitch;          // pitch of the note being played
    u8  prev_pitch;     // pitch of the previous note, for slides
    u16 acc;            // samples played of the current note
    u16 note_len;       // samples per note
    u16 started;        // frame the sfx started, to pick a channel to steal
} Channel;

static Channel chans[CHANNELS];

// AY chip and tone channel for each logical channel
static const u8 chan_chip[CHANNELS] = { 0, 0, 0, 2, 1, 1, 1 };
static const u8 chan_tone[CHANNELS] = { 0, 1, 2, 0, 0, 1, 2 };

static const u16 periods[64] = {
    1672, 1578, 1490, 1406, 1327, 1253, 1182, 1116,
    1053,  994,  939,  886,  836,  789,  745,  703,
     664,  626,  591,  558,  527,  497,  469,  443,
     418,  395,  372,  352,  332,  313,  296,  279,
     263,  249,  235,  221,  209,  197,  186,  176,
     166,  157,  148,  140,  132,  124,  117,  111,
     105,   99,   93,   88,   83,   78,   74,   70,
      66,   62,   59,   55,   52,   49,   47,   44,
};

// sfx volume 0-7 to AY volume 0-15 (the AY scale is logarithmic)
static const u8 volumes[8] = { 0, 7, 9, 11, 12, 13, 14, 15 };

static u16 samples_per_frame;
static u16 frame;

static s8  music_pattern;       // -1 when no music
static u32 music_elapsed;
static u32 music_length;

// AY register shadows: 0-5 periods, 6 noise, 7 mixer, 8-10 volumes
static u8 ay_regs[3][11];

///////////////////////////////////////////////////////////////////////////////

static void ay_write(u8 chip, u8 reg, u8 val)
{
    IO_FFFD = 0xFF - chip;
    IO_FFFD = reg;
    IO_BFFD = val;
}

static void chan_start(Channel* c, u8 id)
{
    const u8* d = SFX_DATA(id);
    u8 speed = d[65] ? d[65] : 1;
    c->data = d;
    c->id = id;
    c->note = 0;
    c->acc = 0;
    c->note_len = speed * NOTE_SAMPLES;
    c->loop_start = d[66];
    c->loop_end = d[67];
    c->len = (c->loop_end == 0 && c->loop_start > 0) ? c->loop_start : 32;
    c->pitch = c->prev_pitch = d[0] & 63;
    c->started = frame;
}

static bool chan_loops(const Channel* c)
{
    return c->loop_start < c->loop_end;
}

void sfx(u8 n)
{
    u8 i, pick = 0xFF;
    if (n >= SFX_COUNT) return;
    // restart the same effect, else take a free channel, else the oldest
    for (i = FIRST_SFX_CHAN; i < CHANNELS; ++i)
        if (chans[i].data && chans[i].id == n) pick = i;
    if (pick == 0xFF)
        for (i = FIRST_SFX_CHAN; i < CHANNELS; ++i)
            if (!chans[i].data) { pick = i; break; }
    if (pick == 0xFF) {
        pick = FIRST_SFX_CHAN;
        for (i = FIRST_SFX_CHAN + 1; i < CHANNELS; ++i)
            if ((u16)(frame - chans[i].started) > (u16)(frame - chans[pick].started)) pick = i;
    }
    chan_start(&chans[pick], n);
}

///////////////////////////////////////////////////////////////////////////////
// music

static void music_start_pattern(u8 p)
{
    const u8* pat = MUSIC + p * 4;
    u8 k;
    s8 master = -1;
    music_pattern = p;
    music_elapsed = 0;
    music_length = 0;
    for (k = 0; k < 4; ++k) {
        Channel* c = &chans[k];
        if (pat[k] & 0x40) {
            c->data = 0;
            continue;
        }
        chan_start(c, pat[k] & 0x3F);
        if (master < 0 && !chan_loops(c)) master = k;
    }
    // the pattern lasts as long as its first non-looping channel,
    // or the longest channel when every channel loops
    for (k = 0; k < 4; ++k) {
        Channel* c = &chans[k];
        u32 len;
        if (!c->data) continue;
        len = (u32)(chan_loops(c) ? 32 : c->len) * c->note_len;
        if (master == (s8)k) { music_length = len; break; }
        if (master < 0 && len > music_length) music_length = len;
    }
    if (music_length == 0) music_stop();
}

void music_play(u8 pattern)
{
    music_start_pattern(pattern);
}

bool music_playing(void)
{
    return music_pattern >= 0;
}

void music_stop(void)
{
    u8 k;
    music_pattern = -1;
    for (k = 0; k < 4; ++k) chans[k].data = 0;
}

static void music_update(void)
{
    const u8* pat;
    s8 p;
    if (music_pattern < 0) return;
    music_elapsed += samples_per_frame;
    if (music_elapsed < music_length) return;

    pat = MUSIC + music_pattern * 4;
    if (pat[2] & 0x80) {                // stop at end of pattern
        music_stop();
        return;
    }
    p = music_pattern;
    if (pat[1] & 0x80) {                // loop back to the last loop start
        while (p > 0 && !(MUSIC[p * 4] & 0x80)) --p;
    } else {
        ++p;
        if (p >= 64) { music_stop(); return; }
    }
    music_start_pattern(p);
}

///////////////////////////////////////////////////////////////////////////////
// channel playback

static void chan_output(u8 i)
{
    Channel* c = &chans[i];
    u8* regs = ay_regs[chan_chip[i]];
    u8 tone = chan_tone[i];
    u8 lo, hi, pitch, wave, vol, fx, prog;
    u16 period;

    if (!c->data) {
        regs[8 + tone] = 0;
        return;
    }

    lo = c->data[c->note * 2];
    hi = c->data[c->note * 2 + 1];
    pitch = lo & 63;
    wave = ((lo >> 6) | (hi << 2)) & 7;
    vol = (hi >> 1) & 7;
    fx = (hi >> 4) & 7;
    // progress through the note (0-255), needed by slide, drop and fades
    prog = 0;
    if (fx == 1 || fx == 3 || fx == 4 || fx == 5)
        prog = (u8)(((u32)c->acc << 8) / c->note_len);
    c->pitch = pitch;

    switch (fx) {
    case 1:     // slide from the previous note
        {
            s16 from = periods[c->prev_pitch];
            s16 to = periods[pitch];
            period = from + (s16)(((s32)(to - from) * prog) >> 8);
        }
        break;
    case 2:     // vibrato
        period = periods[pitch];
        if (frame & 4) period += period >> 6;
        break;
    case 3:     // drop
        period = periods[(u8)(((u16)pitch * (256 - prog)) >> 8)];
        break;
    case 6:     // fast arpeggio
    case 7:     // slow arpeggio
        {
            u8 n = (c->note & 0x1C) | ((frame >> (fx == 6 ? 1 : 2)) & 3);
            period = periods[c->data[n * 2] & 63];
        }
        break;
    default:
        period = periods[pitch];
        break;
    }
    if (fx == 4) vol = (u8)(((u16)vol * prog + 128) >> 8);
    if (fx == 5) vol = (u8)(((u16)vol * (256 - prog) + 128) >> 8);

    regs[tone * 2] = (u8)period;
    regs[tone * 2 + 1] = (u8)(period >> 8);
    regs[8 + tone] = volumes[vol];

    if (wave == 6) {
        u8 np = 31 - (pitch >> 1);
        regs[6] = np ? np : 1;
        regs[7] = (regs[7] | (1 << tone)) & ~(8 << tone);
    } else {
        regs[7] = (regs[7] & ~(1 << tone)) | (8 << tone);
    }
}

static void chan_advance(Channel* c)
{
    if (!c->data) return;
    c->acc += samples_per_frame;
    while (c->acc >= c->note_len) {
        c->acc -= c->note_len;
        c->prev_pitch = c->pitch;
        ++c->note;
        if (chan_loops(c) && c->note >= c->loop_end) {
            c->note = c->loop_start;
        } else if (c->note >= c->len) {
            c->data = 0;
            return;
        }
        c->pitch = c->data[c->note * 2] & 63;
    }
}

void sound_update(void)
{
    u8 i, chip, r;
    ++frame;
    for (chip = 0; chip < 3; ++chip) ay_regs[chip][7] = 0x3F;
    for (i = 0; i < CHANNELS; ++i) chan_output(i);
    for (chip = 0; chip < 3; ++chip)
        for (r = 0; r < 11; ++r) ay_write(chip, r, ay_regs[chip][r]);
    music_update();
    for (i = 0; i < CHANNELS; ++i) chan_advance(&chans[i]);
}

void sound_init(bool hz60)
{
    u8 i;
    samples_per_frame = hz60 ? 368 : 441;
    music_pattern = -1;
    for (i = 0; i < CHANNELS; ++i) chans[i].data = 0;
}
