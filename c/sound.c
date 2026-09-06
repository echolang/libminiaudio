#include "internal.h"

#include <stdlib.h>

static uint32_t sound_flags(
    uint32_t stream,
    uint32_t decode,
    uint32_t looping,
    uint32_t no_pitch,
    uint32_t no_spatialization
)
{
    uint32_t flags = 0;

    if (stream) {
        flags |= MA_SOUND_FLAG_STREAM;
    }

    if (decode) {
        flags |= MA_SOUND_FLAG_DECODE;
    }

    if (looping) {
        flags |= MA_SOUND_FLAG_LOOPING;
    }

    if (no_pitch) {
        flags |= MA_SOUND_FLAG_NO_PITCH;
    }

    if (no_spatialization) {
        flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    }

    return flags;
}

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
)
{
    eco_ma_sound *sound;
    ma_result result;

    if (engine == NULL || path == NULL || out == NULL) {
        return MA_INVALID_ARGS;
    }

    *out = NULL;
    sound = (eco_ma_sound *)calloc(1, sizeof(*sound));
    if (sound == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_sound_init_from_file(
        as_engine(engine),
        path,
        sound_flags(stream, decode, looping, no_pitch, no_spatialization),
        as_group(group),
        NULL,
        &sound->sound
    );
    if (result != MA_SUCCESS) {
        free(sound);
        return result;
    }

    *out = sound;
    return MA_SUCCESS;
}

int32_t eco_ma_sound_init_from_waveform(
    eco_ma_engine *engine,
    int32_t type,
    double frequency,
    uint32_t no_pitch,
    uint32_t no_spatialization,
    eco_ma_sound_group *group,
    eco_ma_sound **out
)
{
    eco_ma_sound *sound;
    ma_engine *inner;
    ma_waveform_config config;
    ma_uint32 channels;
    ma_uint32 sample_rate;
    ma_uint32 flags;
    ma_result result;

    if (engine == NULL || out == NULL) {
        return MA_INVALID_ARGS;
    }

    *out = NULL;

    if (frequency <= 0.0) {
        return MA_INVALID_ARGS;
    }

    if (type < ma_waveform_type_sine || type > ma_waveform_type_sawtooth) {
        return MA_INVALID_ARGS;
    }

    inner = as_engine(engine);
    channels = ma_engine_get_channels(inner);
    sample_rate = ma_engine_get_sample_rate(inner);
    if (channels == 0 || sample_rate == 0) {
        return MA_INVALID_OPERATION;
    }

    sound = (eco_ma_sound *)calloc(1, sizeof(*sound));
    if (sound == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    config = ma_waveform_config_init(
        ma_format_f32,
        channels,
        sample_rate,
        (ma_waveform_type)type,
        1.0,
        frequency
    );

    result = ma_waveform_init(&config, &sound->wave);
    if (result != MA_SUCCESS) {
        free(sound);
        return result;
    }

    flags = MA_SOUND_FLAG_LOOPING;
    if (no_pitch) {
        flags |= MA_SOUND_FLAG_NO_PITCH;
    }
    if (no_spatialization) {
        flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    }

    result = ma_sound_init_from_data_source(
        inner,
        &sound->wave,
        flags,
        as_group(group),
        &sound->sound
    );
    if (result != MA_SUCCESS) {
        ma_waveform_uninit(&sound->wave);
        free(sound);
        return result;
    }

    sound->flags = ECO_SOUND_WAVEFORM;
    *out = sound;
    return MA_SUCCESS;
}

void eco_ma_sound_uninit(eco_ma_sound *sound)
{
    if (sound == NULL) {
        return;
    }

    ma_sound_uninit(&sound->sound);
    if (sound->flags & ECO_SOUND_WAVEFORM) {
        ma_waveform_uninit(&sound->wave);
    }
    free(sound);
}

void eco_ma_sound_get_position(eco_ma_sound *sound, eco_ma_vec3 *out)
{
    if (sound == NULL) {
        return;
    }

    store_xyz(out, ma_sound_get_position(as_sound(sound)));
}

static int32_t waveform_of(eco_ma_sound *sound, ma_waveform **out)
{
    if (sound == NULL || out == NULL) {
        return MA_INVALID_ARGS;
    }

    if ((sound->flags & ECO_SOUND_WAVEFORM) == 0) {
        return MA_INVALID_OPERATION;
    }

    *out = &sound->wave;
    return MA_SUCCESS;
}

int32_t eco_ma_sound_set_frequency(eco_ma_sound *sound, double frequency)
{
    ma_waveform *wave;
    int32_t code;

    if (frequency <= 0.0) {
        return MA_INVALID_ARGS;
    }

    code = waveform_of(sound, &wave);
    if (code != MA_SUCCESS) {
        return code;
    }

    return ma_waveform_set_frequency(wave, frequency);
}

int32_t eco_ma_sound_get_frequency(eco_ma_sound *sound, double *out)
{
    ma_waveform *wave;
    int32_t code;

    if (out == NULL) {
        return MA_INVALID_ARGS;
    }

    code = waveform_of(sound, &wave);
    if (code != MA_SUCCESS) {
        return code;
    }

    *out = wave->config.frequency;
    return MA_SUCCESS;
}

int32_t eco_ma_sound_set_waveform(eco_ma_sound *sound, int32_t type)
{
    ma_waveform *wave;
    int32_t code;

    if (type < ma_waveform_type_sine || type > ma_waveform_type_sawtooth) {
        return MA_INVALID_ARGS;
    }

    code = waveform_of(sound, &wave);
    if (code != MA_SUCCESS) {
        return code;
    }

    return ma_waveform_set_type(wave, (ma_waveform_type)type);
}

int32_t eco_ma_sound_get_waveform(eco_ma_sound *sound, int32_t *out)
{
    ma_waveform *wave;
    int32_t code;

    if (out == NULL) {
        return MA_INVALID_ARGS;
    }

    code = waveform_of(sound, &wave);
    if (code != MA_SUCCESS) {
        return code;
    }

    *out = (int32_t)wave->config.type;
    return MA_SUCCESS;
}

int32_t eco_ma_sound_set_amplitude(eco_ma_sound *sound, double amplitude)
{
    ma_waveform *wave;
    int32_t code;

    code = waveform_of(sound, &wave);
    if (code != MA_SUCCESS) {
        return code;
    }

    return ma_waveform_set_amplitude(wave, amplitude);
}

int32_t eco_ma_sound_get_amplitude(eco_ma_sound *sound, double *out)
{
    ma_waveform *wave;
    int32_t code;

    if (out == NULL) {
        return MA_INVALID_ARGS;
    }

    code = waveform_of(sound, &wave);
    if (code != MA_SUCCESS) {
        return code;
    }

    *out = wave->config.amplitude;
    return MA_SUCCESS;
}
