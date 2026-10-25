///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - game logic, ported from the version by bocianu
//
//  The structure follows the Lua code closely: function names in comments
//  refer to the originals. Map tiles keep their original numbers.
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "game.h"
#include "data.h"
#include "fx.h"
#include "gen_assets.h"
#include "hw.h"
#include "sound.h"

///////////////////////////////////////////////////////////////////////////////
// state

u8   gamestate;
u16  frame;
bool cheat_mode;

u8   map[MAP_ROWS][MAP_W];
u8   map_version;
u8   level;
s8   rlives;
u32  score;
u8   rscrews;
u8   rkeys;
u16  rammo;
u8   rx2;
u8   ry;
u8   robbo_frame;
bool robbo_flip;
bool robbo_visible;
s8   map_ypos;
s8   map_yoff;
u8   curtain;
u8   blink_back;
u8   bg_colour;
bool swap_anim;
bool swap_ship;

static s8   map_ytarget;
static u8   rdir;           // 1-4: left right up down
static u8   rd;             // input delay
static u8   rstep;
static bool robbo_alive;
static bool rfired;         // robbo's bullet is in flight
static bool level_start;
static bool wallwalk;
static u8   escape;
static u8   px, py;         // robbo cell before the last move
static u8   prev_keys;

// sprite flags
#define F_PICK      0x01
#define F_PUSH      0x02
#define F_BULLET    0x04    // bullets destroy it
#define F_EXPLODE   0x08    // explosions destroy it
#define F_DEADLY    0x10    // kills robbo on contact
#define F_MOB       0x20
#define F_LIVING    0x40    // killed by the "?" bonus

#define NIL 0xFF            // tileOnDir() off the board

///////////////////////////////////////////////////////////////////////////////
// random numbers

static u16 rng_state = 0x1234;

void rnd_seed(u16 seed)
{
    rng_state = seed ? seed : 1;
}

// integer in [0, n)
u8 rnd(u8 n)
{
    u16 x = rng_state;
    x ^= x << 7;
    x ^= x >> 9;
    x ^= x << 8;
    rng_state = x;
    return n ? (u8)(x % n) : 0;
}

// the original delays use rnd(n) as a float: it counts down ceil(rnd(n)) ticks
static u8 rnd_delay(u8 n)
{
    return 1 + rnd(n);
}

///////////////////////////////////////////////////////////////////////////////
// map access

static u8 mget(u8 x, u8 y)
{
    return (x < MAP_W && y < MAP_ROWS) ? map[y][x] : 0;
}

static void mset(u8 x, u8 y, u8 t)
{
    if (x < MAP_W && y < MAP_ROWS) {
        map[y][x] = t;
        ++map_version;
    }
}

static bool fget(u8 t, u8 mask)
{
    return t < 128 && (tile_flags[t] & mask);
}

static bool on_board(u8 x, u8 y)
{
    return x < MAP_W && y < MAP_ROWS - 1;
}

static u8 dir_x(u8 x, u8 dir)
{
    if (dir == 1) return x - 1;
    if (dir == 2) return x + 1;
    return x;
}

static u8 dir_y(u8 y, u8 dir)
{
    if (dir == 3) return y - 1;
    if (dir == 4) return y + 1;
    return y;
}

static u8 tile_on_dir(u8 x, u8 y, u8 dir)
{
    if (!on_board(x, y)) return NIL;
    return mget(dir_x(x, dir), dir_y(y, dir));
}

static void movetile(u8 x1, u8 y1, u8 x2, u8 y2)
{
    mset(x2, y2, mget(x1, y1));
    mset(x1, y1, 0);
}

static void add_score(u16 s)
{
    if (!cheat_mode) score += s;
}

///////////////////////////////////////////////////////////////////////////////
// mobs

#define MAX_MOBS    80
#define NO_MOB      0xFF

typedef struct
{
    u8   x, y;
    u8   type;
    u8   tile;
    u8   dir;
    u8   speed;
    u8   delay;
    bool ismoving;
    bool fired;
    bool fwd;
    bool alive;
    u8   parent;
    u8   distance;
    u8   offset;
} Mob;

static Mob mobs[MAX_MOBS];
static u8  mob_count;
static u8  mob_remap[MAX_MOBS];

enum {
    MOB_BULLET = 1, MOB_GUN, MOB_GUN_BULLET, MOB_LASER, MOB_BEAM,
    MOB_ROTATING_GUN, MOB_BLASTER, MOB_BLAST, MOB_CREATURE_L, MOB_CREATURE_R,
    MOB_BAT, MOB_BAT_PIF, MOB_EYES, MOB_MOVING_GUN, MOB_LOOSE_CRATE, MOB_BARRIER
};

// addMob() and getMobParams()
static u8 add_mob(u8 x, u8 y, u8 type, u8 dir)
{
    Mob* m;
    if (mob_count >= MAX_MOBS) return NO_MOB;
    m = &mobs[mob_count];
    memset(m, 0, sizeof(Mob));
    m->x = x;
    m->y = y;
    m->type = type;
    m->dir = dir;
    m->alive = true;
    m->parent = NO_MOB;
    m->ismoving = true;
    m->speed = 2;
    m->delay = 2;
    m->tile = 32;
    switch (type) {
    case MOB_BULLET:
    case MOB_GUN_BULLET:
        m->tile = 38 + ((dir - 1) >> 1) * 2;
        m->delay = 0;
        break;
    case MOB_GUN:
    case MOB_ROTATING_GUN:
        m->tile = 15 + dir;
        m->speed = 64;
        m->ismoving = false;
        m->delay = rnd_delay(64);
        break;
    case MOB_LASER:
        m->tile = 19 + dir;
        m->speed = 64;
        m->ismoving = false;
        m->delay = rnd_delay(64);
        break;
    case MOB_BEAM:
        m->ismoving = false;
        m->tile = 42 + ((dir - 1) >> 1) * 2;
        m->speed = 3;
        m->delay = 0;
        break;
    case MOB_BLASTER:
        m->tile = 23 + dir;
        m->speed = 64;
        m->ismoving = false;
        m->delay = rnd_delay(64);
        break;
    case MOB_BLAST:
        m->ismoving = false;
        m->tile = 42 + ((dir - 1) >> 1) * 2;
        m->speed = 4;
        m->delay = 0;
        break;
    case MOB_CREATURE_L:
        m->tile = 30;
        m->speed = 4;
        break;
    case MOB_CREATURE_R:
        m->tile = 36;
        m->speed = 4;
        break;
    case MOB_BAT:
        m->tile = 34;
        m->speed = 4;
        break;
    case MOB_BAT_PIF:
        m->tile = 60;
        m->speed = 4;
        break;
    case MOB_EYES:
        m->tile = 32;
        m->speed = 7;
        break;
    case MOB_MOVING_GUN:
        m->tile = 28;
        m->speed = 8;
        m->delay = 8;
        break;
    case MOB_LOOSE_CRATE:
        m->tile = 7;
        m->speed = 4;
        m->delay = 4;
        break;
    case MOB_BARRIER:
        m->tile = T_BARRIER;
        m->speed = 4;
        m->delay = 4;
        m->ismoving = false;
        break;
    }
    return mob_count++;
}

static void kill_mob(u8 x, u8 y)
{
    u8 i;
    for (i = 0; i < mob_count; ++i) {
        Mob* m = &mobs[i];
        if (m->alive && m->x == x && m->y == y) {
            m->alive = false;
            mset(x, y, 0);
            return;
        }
    }
}

static void move_mob(u8 x, u8 y, u8 xx, u8 yy)
{
    u8 i;
    for (i = 0; i < mob_count; ++i) {
        Mob* m = &mobs[i];
        if (m->alive && m->x == x && m->y == y) {
            m->x = xx;
            m->y = yy;
            return;
        }
    }
}

// drop dead mobs, keeping order and parent links
static void compact_mobs(void)
{
    u8 i, w = 0;
    for (i = 0; i < mob_count; ++i) {
        if (mobs[i].alive) {
            mob_remap[i] = w;
            if (w != i) mobs[w] = mobs[i];
            ++w;
        } else {
            mob_remap[i] = NO_MOB;
        }
    }
    mob_count = w;
    for (i = 0; i < w; ++i)
        if (mobs[i].parent != NO_MOB) mobs[i].parent = mob_remap[mobs[i].parent];
}

static bool is_mob(u8 t)
{
    return fget(t, F_MOB);
}

///////////////////////////////////////////////////////////////////////////////
// explosions

typedef struct
{
    u8 x, y;
    u8 step;    // 0-14, animation frame is 55 + step / 3
    u8 item;    // tile left behind
} Explosion;

typedef struct
{
    u8   x, y;
    u8   wait;
    bool alive;
} Pending;

#define MAX_EXPLOSIONS  128
#define MAX_PENDING     48

static Explosion explosions[MAX_EXPLOSIONS];
static u8        explosion_count;
static Pending   pending[MAX_PENDING];
static u8        pending_count;

static void add_explosion(u8 x, u8 y, u8 step, u8 item)
{
    Explosion* e;
    if (explosion_count >= MAX_EXPLOSIONS) return;
    e = &explosions[explosion_count++];
    e->x = x;
    e->y = y;
    e->step = step;
    e->item = item;
}

static void explode_later(u8 x, u8 y)
{
    Pending* p;
    if (pending_count >= MAX_PENDING) return;
    p = &pending[pending_count++];
    p->x = x;
    p->y = y;
    p->wait = 4;
    p->alive = true;
}

static void kill_robbo(void);

static const s8 around_dx[8] = {  0,  1, 1, 1, 0, -1, -1, -1 };
static const s8 around_dy[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };

// explodeNow()
static void explode_now(u8 ex, u8 ey)
{
    u8 i;
    sfx(SFX_EXPLOSION);
    fx_burst(FX_EXPLOSION, ex, ey);
    for (i = 0; i < 8; ++i) {
        u8 x = ex + around_dx[i];
        u8 y = ey + around_dy[i];
        u8 t = mget(x, y);
        if (fget(t, F_BULLET | F_EXPLODE) || t == 0) {
            mset(x, y, 0);
            add_explosion(x, y, rnd(6), 0);
            if (t == T_BOMB) explode_later(x, y);
        }
        if (is_mob(t)) kill_mob(x, y);
        if (t == T_ROBBO) kill_robbo();
    }
}

// updateExplosions()
static void update_explosions(void)
{
    u8 i, w;

    for (i = 0; i < pending_count; ++i) {
        Pending* p = &pending[i];
        if (!p->alive) continue;
        if (p->wait > 0) {
            --p->wait;
        } else {
            p->alive = false;
            explode_now(p->x, p->y);
        }
    }
    for (i = 0, w = 0; i < pending_count; ++i)
        if (pending[i].alive) pending[w++] = pending[i];
    pending_count = w;

    for (i = 0, w = 0; i < explosion_count; ++i) {
        Explosion* e = &explosions[i];
        if (e->step == 15) {
            mset(e->x, e->y, e->item);
            continue;
        }
        mset(e->x, e->y, 55 + e->step / 3);
        ++e->step;
        if (w != i) explosions[w] = *e;
        ++w;
    }
    explosion_count = w;
}

///////////////////////////////////////////////////////////////////////////////
// hits

// bulletHit()
static void bullet_hit(u8 x, u8 y, u8 tile)
{
    u8 item = 0;
    sfx(SFX_PUFF);
    fx_burst(FX_HIT, x, y);
    if (tile == T_BOMB) explode_now(x, y);
    if (tile == T_QUESTION) {
        item = qitems[rnd(32)];
        if (item == 16) {
            u8 m = add_mob(x, y, MOB_ROTATING_GUN, rnd(4) + 1);
            item = (m != NO_MOB) ? mobs[m].tile : 0;
        }
        if (item == 32) add_mob(x, y, MOB_EYES, 0);
        if (item == 13) {
            u8 i;
            for (i = 0; i < mob_count; ++i) {
                Mob* m = &mobs[i];
                if (m->alive && fget(m->tile, F_LIVING)) kill_mob(m->x, m->y);
            }
            item = T_LIFE;
        }
    }
    if (is_mob(tile)) kill_mob(x, y);
    if (tile == T_ROBBO) kill_robbo();
    add_explosion(x, y, 6, item);
}

// blastHit()
static void blast_hit(u8 x, u8 y, u8 tile)
{
    sfx(SFX_PUFF);
    if (tile == T_BOMB) explode_now(x, y);
    if (is_mob(tile)) kill_mob(x, y);
    if (tile == T_ROBBO) kill_robbo();
}

// rearm the laser or blaster that fired a beam
static void rearm_parent(Mob* m)
{
    Mob* p;
    if (m->parent == NO_MOB) return;
    p = &mobs[m->parent];
    p->delay = rnd_delay(p->speed);
    p->fired = false;
}

// mobCollided()
static void mob_collided(Mob* m, u8 cx, u8 cy)
{
    u8 ct = mget(cx, cy);
    switch (m->type) {
    case MOB_BULLET:
    case MOB_GUN_BULLET:
        if (fget(ct, F_BULLET)) bullet_hit(cx, cy, ct);
        if (m->tile == mget(m->x, m->y)) add_explosion(m->x, m->y, 9, 0);
        if (m->type == MOB_BULLET) rfired = false;
        m->alive = false;
        break;
    case MOB_BEAM:
        if (m->fwd) {
            if (fget(ct, F_BULLET)) bullet_hit(cx, cy, ct);
            m->dir = revdir[m->dir];
            m->fwd = false;
        } else {
            sfx(SFX_PUFF);
            if (m->parent != NO_MOB) {
                Mob* p = &mobs[m->parent];
                if (m->x != p->x || m->y != p->y) add_explosion(m->x, m->y, 9, 0);
                rearm_parent(m);
            }
            m->alive = false;
        }
        break;
    case MOB_BLAST:
        if (fget(ct, F_BULLET)) blast_hit(cx, cy, ct);
        else m->offset = 2;
        break;
    case MOB_CREATURE_L:
        m->dir = rpath[m->dir];
        break;
    case MOB_CREATURE_R:
        m->dir = lpath[m->dir];
        break;
    case MOB_BAT:
    case MOB_BAT_PIF:
    case MOB_MOVING_GUN:
        m->dir = revdir[m->dir];
        break;
    case MOB_LOOSE_CRATE:
        if (fget(ct, F_BULLET)) bullet_hit(cx, cy, ct);
        m->alive = false;
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// barriers: a row segment the barrier mob runs through, right to left

static u8 bar_left[MAP_ROWS];
static u8 bar_right[MAP_ROWS];

// updateBareers()
static void update_barriers(void)
{
    u8 x, y, i;
    for (y = 0; y < MAP_ROWS; ++y) {
        if (bar_left[y] == NIL) continue;
        for (x = bar_left[y] + 1; x < bar_right[y]; ++x)
            if (map[y][x] == T_BARRIER) {
                map[y][x] = 0;
                ++map_version;
            }
    }
    for (i = 0; i < mob_count; ++i)
        if (mobs[i].alive && mobs[i].type == MOB_BARRIER) mset(mobs[i].x, mobs[i].y, T_BARRIER);
}

///////////////////////////////////////////////////////////////////////////////
// updateMobs()

static void update_mobs(void)
{
    u8 i;
    for (i = 0; i < mob_count; ++i) {
        Mob* m = &mobs[i];
        u8 nx, ny, nt;
        if (!m->alive) continue;
        if (m->delay > 0) {
            --m->delay;
            continue;
        }
        nx = dir_x(m->x, m->dir);
        ny = dir_y(m->y, m->dir);
        nt = mget(nx, ny);

        if (m->ismoving) {
            m->delay = m->speed;
            if (nt == 0 && on_board(nx, ny)) {
                if (mget(m->x, m->y) == m->tile) movetile(m->x, m->y, nx, ny);
                else mset(nx, ny, m->tile);
                m->x = nx;
                m->y = ny;
            } else {
                mob_collided(m, nx, ny);
            }
        }

        switch (m->type) {
        case MOB_BARRIER:
            m->delay = m->speed;
            if (nx == bar_left[ny]) nx = bar_right[ny] - 1;
            m->x = nx;
            // the original code also tried to kill a creature here, but with
            // stale coordinates, so in practice it never did anything
            if (nx * 2 == rx2 && ny == ry && robbo_alive) kill_robbo();
            break;

        case MOB_CREATURE_L:
            if (tile_on_dir(m->x, m->y, lpath[m->dir]) == 0) m->dir = lpath[m->dir];
            break;

        case MOB_CREATURE_R:
            if (tile_on_dir(m->x, m->y, rpath[m->dir]) == 0) m->dir = rpath[m->dir];
            break;

        case MOB_EYES:
            {
                u8 dirs[4];
                u8 n = 0;
                if (ry < m->y) dirs[n++] = 3;
                if (ry > m->y) dirs[n++] = 4;
                if (rx2 < m->x * 2) dirs[n++] = 1;
                if (rx2 > m->x * 2) dirs[n++] = 2;
                if (n) {
                    u8 cdir = dirs[rnd(n)];
                    if (tile_on_dir(m->x, m->y, cdir) == 0) m->dir = cdir;
                }
            }
            break;

        case MOB_GUN:
            m->delay = 4 + rnd_delay(m->speed);
            sfx(SFX_PUFF);
            add_mob(m->x, m->y, MOB_GUN_BULLET, m->dir);
            break;

        case MOB_BAT_PIF:
            if (rnd(10) < 2) {
                sfx(SFX_PUFF);
                add_mob(m->x, m->y, MOB_GUN_BULLET, 4);
            }
            break;

        case MOB_MOVING_GUN:
            if (rnd(10) < 3) {
                sfx(SFX_PUFF);
                add_mob(m->x, m->y, MOB_GUN_BULLET, 3);
            }
            break;

        case MOB_LASER:
            if (!m->fired) {
                u8 b = add_mob(m->x, m->y, MOB_BEAM, m->dir);
                if (b != NO_MOB) {
                    m->fired = true;
                    mobs[b].fwd = true;
                    mobs[b].parent = i;
                }
            }
            break;

        case MOB_BEAM:
            m->delay = m->speed;
            if (m->fwd ? (nt == 0 && on_board(nx, ny)) : (nt == m->tile)) {
                if (!m->fwd) mset(m->x, m->y, 0);
                m->x = nx;
                m->y = ny;
                mset(nx, ny, m->tile);
            } else {
                mob_collided(m, nx, ny);
            }
            break;

        case MOB_BLASTER:
            if (!m->fired) {
                u8 b = add_mob(m->x, m->y, MOB_BLAST, m->dir);
                if (b != NO_MOB) {
                    m->fired = true;
                    mobs[b].offset = 1;
                    mobs[b].parent = i;
                }
            }
            break;

        case MOB_BLAST:
            {
                u8 bx, by, bf, k, back;
                bool free = nt == 0 && on_board(nx, ny);
                m->delay = m->speed;
                if (m->offset == 1) {
                    if (free || fget(nt, F_BULLET)) {
                        ++m->distance;
                        m->x = nx;
                        m->y = ny;
                    }
                    if (!free) mob_collided(m, nx, ny);
                } else {
                    ++m->offset;
                }
                bx = m->x;
                by = m->y;
                bf = m->offset;
                back = revdir[m->dir];
                for (k = 0; k < m->distance; ++k) {
                    mset(bx, by, blast_tiles[bf - 1]);
                    bx = dir_x(bx, back);
                    by = dir_y(by, back);
                    if (++bf > 8) break;
                }
                if (m->offset > 7) {
                    rearm_parent(m);
                    m->alive = false;
                }
            }
            break;

        case MOB_ROTATING_GUN:
            {
                u8 r;
                m->delay = 4 + rnd_delay(m->speed);
                r = rnd(5);
                if (r == 0) m->dir = rpath[m->dir];
                else if (r == 1) m->dir = lpath[m->dir];
                else add_mob(m->x, m->y, MOB_GUN_BULLET, m->dir);
                if (m->tile != 15 + m->dir) {
                    m->tile = 15 + m->dir;
                    mset(m->x, m->y, m->tile);
                }
            }
            break;
        }
    }
    compact_mobs();
}

///////////////////////////////////////////////////////////////////////////////
// teleports

typedef struct
{
    u8 num, x, y;
} Teleport;

#define MAX_TELEPORTS 24

static Teleport teleports[MAX_TELEPORTS];
static u8       teleport_count;
static u8       target_x, target_y;

static bool free_cell(u8 x, u8 y)
{
    target_x = x;
    target_y = y;
    return mget(x, y) == 0 && on_board(x, y);
}

// findTeleportTarget(): sets target_x, target_y
static bool find_teleport_target(u8 tx, u8 ty, u8 dir)
{
    u8 pairs[MAX_TELEPORTS];
    u8 n = 0, i, k, num = NIL;

    for (i = 0; i < teleport_count; ++i)
        if (teleports[i].x == tx && teleports[i].y == ty) num = teleports[i].num;

    // other teleports with the same number: those below first
    for (i = 0; i < teleport_count; ++i) {
        Teleport* t = &teleports[i];
        if (t->num == num && (t->x != tx || t->y != ty) && t->y > ty) pairs[n++] = i;
    }
    for (i = 0; i < teleport_count; ++i) {
        Teleport* t = &teleports[i];
        if (t->num == num && (t->x != tx || t->y != ty) && t->y <= ty) pairs[n++] = i;
    }

    if (n > 0) {
        // straight through first, then any other side
        for (i = 0; i < n; ++i) {
            Teleport* t = &teleports[pairs[i]];
            if (free_cell(dir_x(t->x, dir), dir_y(t->y, dir))) return true;
        }
        for (i = 0; i < n; ++i) {
            Teleport* t = &teleports[pairs[i]];
            for (k = 0; k < 3; ++k) {
                u8 d = (k == 0) ? revdir[dir] : (k == 1) ? lpath[dir] : rpath[dir];
                if (free_cell(dir_x(t->x, d), dir_y(t->y, d))) return true;
            }
        }
    } else {
        // no pair: come out of the same teleport
        for (k = 0; k < 4; ++k) {
            u8 d = (k == 0) ? dir : (k == 1) ? revdir[dir] : (k == 2) ? rpath[dir] : lpath[dir];
            if (free_cell(dir_x(tx, d), dir_y(ty, d))) return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// timed events, the anim[] closures of the original

enum {
    EV_NONE,
    EV_LEVEL_STARTED,
    EV_LANDING_SFX,
    EV_START_PUFF,
    EV_CLOSE_CURTAIN,
    EV_NEXT_LEVEL,
    EV_EXIT_PUFF,
    EV_DEATH_EXPLOSION,
    EV_AFTER_DEATH,
    EV_TELEPORT_OUT,
    EV_TELEPORT_IN,
};

#define ANIM_LEN 37

static s8 animation;
static u8 anim[ANIM_LEN];

static void load_level(void);

static void run_event(u8 ev)
{
    switch (ev) {
    case EV_LEVEL_STARTED:   level_start = false; break;
    case EV_LANDING_SFX:     sfx(SFX_LANDING); break;
    case EV_START_PUFF:      add_explosion(rx2 >> 1, ry, 9, 0); break;
    case EV_CLOSE_CURTAIN:   curtain += 2; break;
    case EV_EXIT_PUFF:       add_explosion(rx2 >> 1, ry, 6, 0); break;
    case EV_DEATH_EXPLOSION: add_explosion(rx2 >> 1, ry, 0, 0); break;
    case EV_TELEPORT_OUT:    add_explosion(px, py, 9, 0); break;
    case EV_TELEPORT_IN:     add_explosion(rx2 >> 1, ry, 9, T_ROBBO); break;
    case EV_NEXT_LEVEL:
        ++level;
        load_level();
        break;
    case EV_AFTER_DEATH:
        if (rlives < 0) show_title();
        else load_level();
        break;
    }
}

static void curtain_events(void)
{
    u8 i;
    for (i = 2; i <= 16; ++i) anim[i] = EV_CLOSE_CURTAIN;
}

// killRobbo()
static void kill_robbo(void)
{
    // unlike the original, a robbo who is already dead or leaving in
    // his capsule cannot be killed (again)
    if (!robbo_alive) return;
    robbo_alive = false;
    memset(anim, 0, sizeof(anim));
    animation = 36;
    --rlives;
    fx_burst(FX_DEATH, rx2 >> 1, ry);
    sfx(SFX_EXPLOSION);
    anim[36] = EV_DEATH_EXPLOSION;
    curtain_events();
    anim[1] = EV_AFTER_DEATH;
}

///////////////////////////////////////////////////////////////////////////////
// level loading

static u8 bcd2dec(u8 b)
{
    return (b >> 4) * 10 + (b & 15);
}

static u8 code_index(const u8* codes, u8 code)
{
    u8 i;
    for (i = 0; i < 4; ++i)
        if (codes[i] == code) return i + 1;
    return 0;
}

static u8 mob_tile(u8 x, u8 y, u8 type, u8 dir)
{
    u8 m = add_mob(x, y, type, dir);
    return m != NO_MOB ? mobs[m].tile : 0;
}

// loadMap(): decodes the Atari level format
static void load_map(u8 n)
{
    const u8* src = PAGE_WINDOW + (u16)(n & 15) * 512;
    const u8* props = level_props[level - 1];
    u8 x, y, d;

    page_in(PAGE_LEVELS + (n >> 4));

    for (y = 0; y < MAP_ROWS - 1; ++y) {
        for (x = 0; x < MAP_W; ++x) {
            u8 c = *src++;
            u8 t;
            if (c == 0x2A) {                    // player
                t = T_SHIP_ALT;
                rx2 = x * 2;
                ry = y;
            }
            else if (c == 0xA0) t = props[0];           // wall
            else if (c == 0x5C) t = props[0] + 16;      // inverted wall
            else if (c == 0x13) t = T_VOID;
            else if (c == 0x20) t = 0;
            else if (c == 0x23) t = 6;                  // crate
            else if (c == 0x25) t = 5;                  // grass
            else if (c == 0x24) t = T_SCREW;
            else if (c == 0x3D) t = T_KEY;
            else if (c == 0x40) t = T_BOMB;
            else if (c == 0x2B) t = T_LIFE;
            else if (c == 0x21) t = T_AMMO;
            else if (c == 0x14) t = T_SHIP;
            else if (c == 0x7C || c == 0x12) t = T_DOOR;
            else if (c == 0x06) t = 7;                  // loose crate
            else if (c == 0x28) t = T_MAGNET_L;
            else if (c == 0x29) t = T_MAGNET_R;
            else if (c > 0x2F && c < 0x3A) {            // teleport
                if (teleport_count < MAX_TELEPORTS) {
                    Teleport* tp = &teleports[teleport_count++];
                    tp->num = c - 0x2F;
                    tp->x = x;
                    tp->y = y;
                }
                t = T_TELEPORT;
            }
            else if (c > 0x40 && c < 0x45) t = mob_tile(x, y, MOB_CREATURE_L, c - 0x40);
            else if (c > 0x44 && c < 0x49) t = mob_tile(x, y, MOB_CREATURE_R, c - 0x44);
            else if (c > 0x48 && c < 0x4D) t = mob_tile(x, y, MOB_BAT, c - 0x48);
            else if (c > 0x4C && c < 0x4F) t = mob_tile(x, y, MOB_BAT_PIF, c - 0x4C);
            else if (c == 0x26) t = mob_tile(x, y, MOB_EYES, 0);
            else if ((d = code_index(gun_codes, c))) t = mob_tile(x, y, MOB_GUN, d);
            else if (c == 0x0E || c == 0x0D) t = mob_tile(x, y, MOB_MOVING_GUN, c == 0x0D ? 1 : 2);
            else if (c > 0x2B && c < 0x30) t = mob_tile(x, y, MOB_ROTATING_GUN, c - 0x2B);
            else if ((d = code_index(laser_codes, c))) t = mob_tile(x, y, MOB_LASER, d);
            else if ((d = code_index(blaster_codes, c))) t = mob_tile(x, y, MOB_BLASTER, d);
            else if (c == 0x3F) t = T_QUESTION;
            else if (c == 0x11) {                       // barrier left end
                bar_left[y] = x;
                t = props[0];
            }
            else if (c == 0x0F) {                       // barrier
                add_mob(x, y, MOB_BARRIER, 1);
                t = 0;
            }
            else if (c == 0x05) {                       // barrier right end
                bar_right[y] = x;
                t = props[0];
            }
            else t = 60;
            map[y][x] = t;
        }
    }
    // the last row holds level parameters; byte 1 is the screw count
    rscrews = bcd2dec(src[1]);
    page_out();
    memset(map[MAP_ROWS - 1], T_VOID, MAP_W);
    ++map_version;
}

// loadLevel()
static void load_level(void)
{
    rammo = 0;
    rkeys = 0;
    rscrews = 0;
    rdir = 4;
    rd = 0;
    rstep = 0;
    rfired = false;
    mob_count = 0;
    teleport_count = 0;
    explosion_count = 0;
    pending_count = 0;
    memset(bar_left, NIL, sizeof(bar_left));
    memset(bar_right, NIL, sizeof(bar_right));
    memset(anim, 0, sizeof(anim));
    curtain = 16;
    swap_anim = false;
    swap_ship = false;
    if (level > LAST_LEVEL) {
        start_ending();
        return;
    }
    load_map(level - 1);
    robbo_alive = true;
    level_start = true;
    animation = 30;
    anim[1] = EV_LEVEL_STARTED;
    anim[14] = EV_LANDING_SFX;
    anim[12] = EV_START_PUFF;
}

///////////////////////////////////////////////////////////////////////////////
// robbo

static void pick_screw(void)
{
    if (rscrews == 1) {
        blink_back = 6;
        sfx(SFX_LAST_SCREW);
    }
    if (rscrews > 0) --rscrews;
}

static void fire(u8 bx, u8 by, u8 dir)
{
    if (rammo > 0 && add_mob(bx, by, MOB_BULLET, dir) != NO_MOB) {
        sfx(SFX_PISTOL);
        --rammo;
        rfired = true;
    }
}

// checkUserInput()
static void check_user_input(void)
{
    u8 rx = rx2 >> 1;
    s8 dx = 0, dy = 0;

    if (rd > 0) {
        --rd;
        if (btn_state == 0) rd = 0;
    } else {
        if (key_down(KEY_R)) {          // robbo gives up
            if (++escape == 50) kill_robbo();
        } else {
            escape = 0;
        }
        px = rx;
        py = ry;
        if (btn_state & BTN_LEFT) { rdir = 1; dx = -1; }
        else if (btn_state & BTN_RIGHT) { rdir = 2; dx = 1; }
        else if (btn_state & BTN_UP) { rdir = 3; dy = -1; }
        else if (btn_state & BTN_DOWN) { rdir = 4; dy = 1; }

        if (dx || dy) {
            bool move = false;
            u8 tx = rx + dx;
            u8 ty = ry + dy;
            u8 ft = mget(tx, ty);

            if (btn_state & BTN_FIRE) {                         // robbo fires
                if (!rfired) {
                    fire(rx, ry, rdir);
                    rd = 3;
                }
            } else if (on_board(tx, ty)) {                      // robbo tries to move
                if (ft == 0 || wallwalk) {
                    move = true;
                } else if ((ft == T_SHIP && rscrews == 0) || ft == T_SHIP_ALT) {
                    robbo_alive = false;                        // capsule: next level
                    fx_burst(FX_PICKUP, tx, ty);
                    add_score(1000);
                    memset(anim, 0, sizeof(anim));
                    animation = 36;
                    anim[36] = EV_EXIT_PUFF;
                    curtain_events();
                    anim[1] = EV_NEXT_LEVEL;
                    sfx(SFX_EXIT);
                } else if (fget(ft, F_PUSH)) {
                    u8 fx = rx + dx * 2;
                    u8 fy = ry + dy * 2;
                    if (mget(fx, fy) == 0 && on_board(fx, fy)) {
                        sfx(SFX_PUSH);
                        movetile(tx, ty, fx, fy);
                        if (ft == 28) move_mob(tx, ty, fx, fy);   // moving gun
                        if (ft == 7) add_mob(fx, fy, MOB_LOOSE_CRATE, rdir);
                        move = true;
                    }
                } else if (fget(ft, F_PICK)) {
                    mset(tx, ty, 0);
                    move = true;
                    fx_burst(FX_PICKUP, tx, ty);
                    if (ft == T_AMMO) {
                        rammo += 9;
                        add_score(50);
                        sfx(SFX_AMMO);
                    }
                    if (ft == T_SCREW) {
                        add_score(rscrews == 0 ? 250 : 100);
                        pick_screw();
                        sfx(SFX_SCREW);
                    }
                    if (ft == T_KEY) {
                        ++rkeys;
                        add_score(75);
                        sfx(SFX_KEY);
                    }
                    if (ft == T_LIFE) {
                        ++rlives;
                        add_score(200);
                        sfx(SFX_EXTRA_LIFE);
                    }
                } else if (ft == T_DOOR) {
                    if (rkeys > 0) {
                        --rkeys;
                        mset(tx, ty, 0);
                        move = true;
                        add_score(100);
                        sfx(SFX_DOOR);
                    }
                } else if (ft == T_TELEPORT || ft == T_TELEPORT2) {
                    if (find_teleport_target(tx, ty, rdir)) {
                        fx_burst(FX_TELEPORT, px, py);
                        fx_burst(FX_TELEPORT, target_x, target_y);
                        rx2 = target_x * 2;
                        ry = target_y;
                        movetile(px, py, target_x, target_y);
                        sfx(SFX_TELEPORT);
                    }
                    animation = 16;
                    anim[16] = EV_TELEPORT_OUT;
                    anim[12] = EV_TELEPORT_IN;
                }
            }

            if (move) {
                sfx(SFX_STEP);
                ++rstep;
                mset(px, py, 0);
                mset(tx, ty, T_ROBBO);
                rx2 = tx * 2;
                ry = ty;
                rd = 3;
            } else {
                rstep = 0;
            }
        } else {
            rstep = 0;
        }
    }

    robbo_frame = robbo_frames[rdir - 1][rstep & 3][0];
    robbo_flip = robbo_frames[rdir - 1][rstep & 3][1];
}

// checkForCollissions()
static void check_collisions(void)
{
    u8 rx = rx2 >> 1;
    u8 d, t, cx2;

    for (d = 1; d <= 4; ++d) {
        if (fget(tile_on_dir(rx, ry, d), F_DEADLY)) {
            kill_robbo();
            return;
        }
    }
    if (tile_on_dir(rx, ry, 1) == T_MAGNET_L || tile_on_dir(rx, ry, 2) == T_MAGNET_R) {
        kill_robbo();
        return;
    }
    // magnets pull robbo along an empty line, half a cell per update
    cx2 = rx2;
    do {
        t = tile_on_dir(cx2 >> 1, ry, 1);
        if (t == T_MAGNET_L) {
            rd = 10;
            if (rx2 > cx2) --rx2;
        }
        cx2 -= 2;
    } while (t == 0);
    cx2 = rx2;
    do {
        t = tile_on_dir(cx2 >> 1, ry, 2);
        if (t == T_MAGNET_R) {
            rd = 10;
            if (rx2 < cx2) ++rx2;
        }
        cx2 += 2;
    } while (t == 0);
}

///////////////////////////////////////////////////////////////////////////////
// camera

// moveMap()
static void move_map(void)
{
    if (map_ypos != map_ytarget || map_yoff != 0) {
        if (map_yoff != 0) {
            map_yoff += (map_yoff > 0) ? 2 : -2;
            if (map_yoff == 8 || map_yoff == -8) {
                map_ypos -= (map_yoff > 0) ? 1 : -1;
                map_yoff = 0;
            }
        } else {
            if (map_ypos < map_ytarget) map_yoff -= 2;
            if (map_ypos > map_ytarget) map_yoff += 2;
        }
        if (animation > 0 && level_start) ++animation;
    } else {
        // as in the original (10 rows: 2 and 7, last row 21), for VIEW_ROWS rows
        s8 rel = ry - map_ypos;
        s8 last = MAP_ROWS - 1 - VIEW_ROWS;
        if (rel < 2 && map_ypos > 0) map_ytarget -= (map_ypos < 3) ? map_ypos : 3;
        if (rel > VIEW_ROWS - 3 && map_ypos < last)
            map_ytarget += (last - map_ypos < 3) ? last - map_ypos : 3;
    }
}

///////////////////////////////////////////////////////////////////////////////
// cheats (keys, as in the original cheat mode)

static void cheats(void)
{
    u8 keys = 0, hit;
    if (key_down(KEY_N)) keys |= 0x01;
    if (key_down(KEY_B)) keys |= 0x02;
    if (key_down(KEY_J)) keys |= 0x04;
    if (key_down(KEY_S)) keys |= 0x08;
    if (key_down(KEY_K)) keys |= 0x10;
    if (key_down(KEY_L)) keys |= 0x20;
    if (key_down(KEY_W)) keys |= 0x40;
    hit = keys & ~prev_keys;
    prev_keys = keys;

    if (hit & 0x01) {
        if (level <= LAST_LEVEL) ++level;
        load_level();
    }
    if (hit & 0x02) {
        if (level > 1) --level;
        load_level();
    }
    if (hit & 0x04) rammo += 10;
    if (hit & 0x08) pick_screw();
    if (hit & 0x10) ++rkeys;
    if (hit & 0x20) ++rlives;
    if (hit & 0x40) wallwalk = !wallwalk;
}

///////////////////////////////////////////////////////////////////////////////

// initGame() + loadLevel()
void game_start(void)
{
    music_stop();
    rnd_seed(frame * 31 + 7);
    rlives = 8;
    score = 0;
    level = 1;
    map_ypos = 0;
    map_ytarget = 0;
    map_yoff = 0;
    escape = 0;
    level_start = false;
    blink_back = 0;
    robbo_frame = 48;
    robbo_flip = false;
    prev_keys = 0xFF;
    gamestate = STATE_GAME;
    load_level();
}

// updategame()
void game_update(void)
{
    // BREAK exits to the title; cursor keys also press CAPS SHIFT, so it
    // does not count while a direction is held (fire while moving)
    if (key_down(KEY_CAPS) && key_down(KEY_SPACE) &&
        !(btn_state & (BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN))) {
        show_title();
        return;
    }
    if (animation <= 0 && robbo_alive) check_user_input();
    move_map();
    update_mobs();
    update_explosions();
    if ((frame & 7) == 0) {
        swap_anim = !swap_anim;
        if (rscrews == 0) swap_ship = !swap_ship;
    }
    if (curtain > 0) --curtain;
    if (robbo_alive) check_collisions();
    update_barriers();
    if (cheat_mode) cheats();
}

// the event part of drawgame()
void game_draw(void)
{
    if (blink_back > 0) {
        bg_colour = blink_colours[blink_back - 1];
        --blink_back;
    } else if (level <= LAST_LEVEL) {
        bg_colour = level_props[level - 1][1];
    }
    if (animation > 0) {
        u8 ev = anim[animation];
        robbo_visible = false;
        if (ev) {
            run_event(ev);
            if (animation >= 0 && animation < ANIM_LEN) anim[animation] = EV_NONE;
        }
        --animation;
    } else {
        robbo_visible = true;
    }
}
