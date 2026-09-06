#include "internal.h"

#include <stdlib.h>
#include <string.h>

struct eco_ma_pcm {
    ma_pcm_rb rb;
    ma_sound sound;
};

int32_t eco_ma_pcm_init(eco_ma_engine *engine, uint32_t capacity_frames, eco_ma_pcm **out)
{
    eco_ma_pcm *pcm;
    ma_engine *inner;
    ma_uint32 channels;
    ma_uint32 sample_rate;
    ma_result result;

    if (engine == NULL || out == NULL) {
        return MA_INVALID_ARGS;
    }

    *out = NULL;

    if (capacity_frames == 0) {
        return MA_INVALID_ARGS;
    }

    inner = as_engine(engine);
    channels = ma_engine_get_channels(inner);
    sample_rate = ma_engine_get_sample_rate(inner);

    if (channels == 0 || sample_rate == 0) {
        return MA_INVALID_OPERATION;
    }

    pcm = (eco_ma_pcm *)calloc(1, sizeof(*pcm));
    if (pcm == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_pcm_rb_init(ma_format_f32, channels, capacity_frames, NULL, NULL, &pcm->rb);
    if (result != MA_SUCCESS) {
        free(pcm);
        return result;
    }

    ma_pcm_rb_set_sample_rate(&pcm->rb, sample_rate);

    result = ma_sound_init_from_data_source(
        inner,
        &pcm->rb,
        MA_SOUND_FLAG_NO_SPATIALIZATION,
        NULL,
        &pcm->sound
    );
    if (result != MA_SUCCESS) {
        ma_pcm_rb_uninit(&pcm->rb);
        free(pcm);
        return result;
    }

    *out = pcm;
    return MA_SUCCESS;
}

void eco_ma_pcm_uninit(eco_ma_pcm *pcm)
{
    if (pcm == NULL) {
        return;
    }

    ma_sound_uninit(&pcm->sound);
    ma_pcm_rb_uninit(&pcm->rb);
    free(pcm);
}

int32_t eco_ma_pcm_write(eco_ma_pcm *pcm, const float *samples, uint64_t frame_count, uint64_t *written)
{
    uint64_t total;
    const float *src;
    ma_uint32 channels;

    if (pcm == NULL || written == NULL) {
        return MA_INVALID_ARGS;
    }

    *written = 0;

    if (frame_count == 0) {
        return MA_SUCCESS;
    }

    if (samples == NULL) {
        return MA_INVALID_ARGS;
    }

    channels = ma_pcm_rb_get_channels(&pcm->rb);
    if (channels == 0) {
        return MA_INVALID_OPERATION;
    }

    total = 0;
    src = samples;

    while (total < frame_count) {
        uint32_t want;
        uint32_t got;
        void *dst;
        uint64_t remain;
        ma_result result;

        remain = frame_count - total;
        if (remain > 0xffffffffu) {
            want = 0xffffffffu;
        } else {
            want = (uint32_t)remain;
        }

        got = want;
        result = ma_pcm_rb_acquire_write(&pcm->rb, &got, &dst);
        if (result != MA_SUCCESS || got == 0 || dst == NULL) {
            break;
        }

        memcpy(dst, src, (size_t)got * (size_t)channels * sizeof(float));
        result = ma_pcm_rb_commit_write(&pcm->rb, got);
        if (result != MA_SUCCESS) {
            break;
        }

        src += (size_t)got * (size_t)channels;
        total += got;
    }

    *written = total;
    return MA_SUCCESS;
}

uint64_t eco_ma_pcm_writable(eco_ma_pcm *pcm)
{
    if (pcm == NULL) {
        return 0;
    }

    return (uint64_t)ma_pcm_rb_available_write(&pcm->rb);
}

int32_t eco_ma_pcm_start(eco_ma_pcm *pcm)
{
    if (pcm == NULL) {
        return MA_INVALID_ARGS;
    }

    return ma_sound_start(&pcm->sound);
}

int32_t eco_ma_pcm_stop(eco_ma_pcm *pcm)
{
    if (pcm == NULL) {
        return MA_INVALID_ARGS;
    }

    return ma_sound_stop(&pcm->sound);
}

void eco_ma_pcm_set_volume(eco_ma_pcm *pcm, float volume)
{
    if (pcm == NULL) {
        return;
    }

    ma_sound_set_volume(&pcm->sound, volume);
}

float eco_ma_pcm_get_volume(eco_ma_pcm *pcm)
{
    if (pcm == NULL) {
        return 0.0f;
    }

    return ma_sound_get_volume(&pcm->sound);
}

void eco_ma_pcm_set_pan(eco_ma_pcm *pcm, float pan)
{
    if (pcm == NULL) {
        return;
    }

    ma_sound_set_pan(&pcm->sound, pan);
}

float eco_ma_pcm_get_pan(eco_ma_pcm *pcm)
{
    if (pcm == NULL) {
        return 0.0f;
    }

    return ma_sound_get_pan(&pcm->sound);
}

uint32_t eco_ma_pcm_is_playing(eco_ma_pcm *pcm)
{
    if (pcm == NULL) {
        return 0;
    }

    return ma_sound_is_playing(&pcm->sound);
}
