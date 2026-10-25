///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - particle effects (purely visual)
//
//  Up to 32 sprites flying over the board: debris and sparks from
//  explosions, stars when robbo picks something up or teleports. They use
//  their own random numbers, so the game plays exactly the same with or
//  without them.
//
///////////////////////////////////////////////////////////////////////////////

#include "fx.h"
#include "game.h"
#include "gen_assets.h"
#include "hw.h"
#include "render.h"

#define MAX_PARTICLES   32
#define SLOT_PARTICLES  32

// position in 1/8 pixels of the board (world), velocity in 1/8 pixels
typedef struct
{
    s16 x, y;
    s8  vx, vy;
    u8  life;           // ticks left, 0 = free
    u8  kind;
} Particle;

enum { P_SPARK, P_DEBRIS, P_SMOKE, P_STAR };

static Particle parts[MAX_PARTICLES];
static u8 next_part;
static u16 seed = 0x5EED;

static u8 fx_rnd(void)
{
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (u8)seed;
}

// a velocity in -range..range
static s8 spread(u8 range)
{
    return (s8)(fx_rnd() % (range * 2 + 1)) - (s8)range;
}

static void spawn(u8 kind, s16 x, s16 y, s8 vx, s8 vy, u8 life)
{
    Particle* p = &parts[next_part];
    next_part = (next_part + 1) & (MAX_PARTICLES - 1);
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    p->life = life;
    p->kind = kind;
}

void fx_burst(u8 kind, u8 cx, u8 cy)
{
    s16 x = (s16)cx * 128 + 64;         // centre of the cell, 1/8 pixels
    s16 y = (s16)cy * 128 + 64;
    u8 i;
    if (cx >= MAP_W || cy >= MAP_ROWS) return;
    switch (kind) {
    case FX_EXPLOSION:
        for (i = 0; i < 6; ++i) spawn(P_DEBRIS, x, y, spread(14), -(s8)(fx_rnd() % 20) - 6, 22);
        for (i = 0; i < 4; ++i) spawn(P_SPARK, x, y, spread(20), spread(20), 10);
        spawn(P_SMOKE, x, y - 32, spread(3), -4, 26);
        break;
    case FX_HIT:
        for (i = 0; i < 3; ++i) spawn(P_SPARK, x, y, spread(16), spread(16), 7);
        break;
    case FX_PICKUP:
        for (i = 0; i < 6; ++i) {
            static const s8 dx[6] = { 16, 8, -8, -16, -8, 8 };
            static const s8 dy[6] = { 0, -14, -14, 0, 14, 14 };
            spawn(P_STAR, x, y, dx[i], dy[i], 9);
        }
        break;
    case FX_DEATH:
        for (i = 0; i < 8; ++i) spawn(P_DEBRIS, x, y, spread(18), -(s8)(fx_rnd() % 24) - 6, 26);
        for (i = 0; i < 6; ++i) spawn(P_SPARK, x, y, spread(24), spread(24), 12);
        break;
    case FX_TELEPORT:
        for (i = 0; i < 5; ++i) spawn(P_STAR, x + spread(48), y + spread(40), spread(3), -10, 12);
        break;
    }
}

void fx_clear(void)
{
    u8 i;
    for (i = 0; i < MAX_PARTICLES; ++i) {
        parts[i].life = 0;
        sprite_hide(SLOT_PARTICLES + i);
    }
}

void fx_draw(s16 cam, u8 curtain_cols)
{
    s16 left = BOARD_X + curtain_cols * 16;
    u8 i;
    Particle* p = parts;
    for (i = 0; i < MAX_PARTICLES; ++i, ++p) {
        s16 sx, sy;
        u8 pat;
        if (!p->life) continue;
        --p->life;
        p->x += p->vx;
        p->y += p->vy;
        switch (p->kind) {
        case P_DEBRIS:
            if (p->vy < 40) p->vy += 3;         // gravity
            pat = (p->life & 2) ? SPR_DEBRIS : SPR_DEBRIS2;
            break;
        case P_SMOKE:
            pat = p->life > 12 ? SPR_SMOKE : SPR_SMOKE2;
            break;
        case P_STAR:
            p->vx -= p->vx / 4;                 // slow down
            p->vy -= p->vy / 4;
            pat = p->life > 4 ? SPR_STAR : SPR_STAR2;
            break;
        default:
            pat = p->life > 3 ? SPR_SPARK : SPR_SPARK2;
            break;
        }
        sx = BOARD_X + (p->x >> 3) - 8;
        sy = BOARD_Y + (p->y >> 3) - cam - 8;
        if (!p->life || sx < left - 4 || sx > BOARD_X + BOARD_W - 12 ||
            sy < BOARD_Y - 4 || sy > BOARD_Y + BOARD_H - 12) {
            sprite_hide(SLOT_PARTICLES + i);
            continue;
        }
        sprite_show(SLOT_PARTICLES + i, sx, sy, pat, 0, 0);
    }
}
