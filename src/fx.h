///////////////////////////////////////////////////////////////////////////////
//
//  RobboNext - particle effects (HD graphics only, purely visual)
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _fx_h_
#define _fx_h_

#include "types.h"

enum {
    FX_EXPLOSION,       // debris, sparks and smoke
    FX_HIT,             // a few sparks
    FX_PICKUP,          // a ring of stars
    FX_DEATH,           // robbo explodes
    FX_TELEPORT,        // stars rising
};

// start an effect on map cell (cx, cy)
void fx_burst(u8 kind, u8 cx, u8 cy);
void fx_clear(void);
// move and draw; particles left of the curtain (cols covered) are hidden
void fx_draw(s16 cam, u8 curtain_cols);

#endif
