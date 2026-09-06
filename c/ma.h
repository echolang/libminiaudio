#ifndef ECO_MA_H
#define ECO_MA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Echo-facing ABI for the parts Echo cannot say: heap-allocating miniaudio
 * objects (no stable layout), packing options into miniaudio's configs,
 * and unpacking vec3. Everything else is a miniaudio symbol in sys.eco.
 */

typedef struct eco_ma_engine eco_ma_engine;
typedef struct eco_ma_sound eco_ma_sound;
typedef struct eco_ma_sound_group eco_ma_sound_group;
typedef struct eco_ma_pcm eco_ma_pcm;

/*
 * Layout of ma::Vec3 / ma_vec3f: 3 x float32, this order.
 */
typedef struct {
    float x;
    float y;
    float z;
} eco_ma_vec3;

_Static_assert(sizeof(eco_ma_vec3) == 12, "eco_ma_vec3 is 3 x float");
_Static_assert(offsetof(eco_ma_vec3, x) == 0, "x");
_Static_assert(offsetof(eco_ma_vec3, y) == 4, "y");
_Static_assert(offsetof(eco_ma_vec3, z) == 8, "z");

int32_t eco_ma_engine_init(
    uint32_t channels,
    uint32_t sample_rate,
    uint32_t no_auto_start,
    uint32_t no_device,
    eco_ma_engine **out
);
void    eco_ma_engine_uninit(eco_ma_engine *engine);

void eco_ma_engine_listener_get_position(eco_ma_engine *engine, uint32_t index, eco_ma_vec3 *out);
void eco_ma_engine_listener_get_direction(eco_ma_engine *engine, uint32_t index, eco_ma_vec3 *out);
void eco_ma_engine_listener_get_world_up(eco_ma_engine *engine, uint32_t index, eco_ma_vec3 *out);

int32_t eco_ma_sound_init_from_file(
    eco_ma_engine *engine,
    const char *path,
    uint32_t stream,
    uint32_t decode,
    uint32_t looping,
    uint32_t no_pitch,
    uint32_t no_spatialization,
    eco_ma_sound_group *group,
    eco_ma_sound **out
);
int32_t eco_ma_sound_init_from_waveform(
    eco_ma_engine *engine,
    int32_t type,
    double frequency,
    uint32_t no_pitch,
    uint32_t no_spatialization,
    eco_ma_sound_group *group,
    eco_ma_sound **out
);
void    eco_ma_sound_uninit(eco_ma_sound *sound);
void    eco_ma_sound_get_position(eco_ma_sound *sound, eco_ma_vec3 *out);
int32_t eco_ma_sound_set_frequency(eco_ma_sound *sound, double frequency);
int32_t eco_ma_sound_get_frequency(eco_ma_sound *sound, double *out);
int32_t eco_ma_sound_set_waveform(eco_ma_sound *sound, int32_t type);
int32_t eco_ma_sound_get_waveform(eco_ma_sound *sound, int32_t *out);
int32_t eco_ma_sound_set_amplitude(eco_ma_sound *sound, double amplitude);
int32_t eco_ma_sound_get_amplitude(eco_ma_sound *sound, double *out);

int32_t eco_ma_sound_group_init(eco_ma_engine *engine, eco_ma_sound_group **out);
void    eco_ma_sound_group_uninit(eco_ma_sound_group *group);

int32_t eco_ma_pcm_init(eco_ma_engine *engine, uint32_t capacity_frames, eco_ma_pcm **out);
void    eco_ma_pcm_uninit(eco_ma_pcm *pcm);
int32_t eco_ma_pcm_write(eco_ma_pcm *pcm, const float *samples, uint64_t frame_count, uint64_t *written);
uint64_t eco_ma_pcm_writable(eco_ma_pcm *pcm);
int32_t eco_ma_pcm_start(eco_ma_pcm *pcm);
int32_t eco_ma_pcm_stop(eco_ma_pcm *pcm);
void    eco_ma_pcm_set_volume(eco_ma_pcm *pcm, float volume);
float   eco_ma_pcm_get_volume(eco_ma_pcm *pcm);
void    eco_ma_pcm_set_pan(eco_ma_pcm *pcm, float pan);
float   eco_ma_pcm_get_pan(eco_ma_pcm *pcm);
uint32_t eco_ma_pcm_is_playing(eco_ma_pcm *pcm);

#ifdef __cplusplus
}
#endif

#endif
