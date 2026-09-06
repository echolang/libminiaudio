#ifndef ECO_MA_INTERNAL_H
#define ECO_MA_INTERNAL_H

#include "miniaudio.h"
#include "ma.h"

#define ECO_SOUND_WAVEFORM 1u

struct eco_ma_sound {
    ma_sound sound; /* first: ma_sound_* on this pointer still works */
    uint32_t flags;
    ma_waveform wave;
};

static inline ma_engine *as_engine(eco_ma_engine *p)
{
    return (ma_engine *)p;
}

static inline ma_sound *as_sound(eco_ma_sound *p)
{
    return &p->sound;
}

static inline ma_sound_group *as_group(eco_ma_sound_group *p)
{
    return (ma_sound_group *)p;
}

static inline void store_xyz(eco_ma_vec3 *out, ma_vec3f v)
{
    if (out == NULL) {
        return;
    }

    out->x = v.x;
    out->y = v.y;
    out->z = v.z;
}

#endif
