#include "internal.h"

#include <stdlib.h>

int32_t eco_ma_engine_init(
    uint32_t channels,
    uint32_t sample_rate,
    uint32_t no_auto_start,
    uint32_t no_device,
    eco_ma_engine **out
)
{
    ma_engine_config c;
    ma_engine *engine;
    ma_result result;

    if (out == NULL) {
        return MA_INVALID_ARGS;
    }

    *out = NULL;
    c = ma_engine_config_init();

    if (channels > 0) {
        c.channels = channels;
    }

    if (sample_rate > 0) {
        c.sampleRate = sample_rate;
    }

    c.noAutoStart = no_auto_start ? MA_TRUE : MA_FALSE;
    c.noDevice = no_device ? MA_TRUE : MA_FALSE;

    if (c.noDevice) {
        if (c.channels == 0) {
            c.channels = 2;
        }

        if (c.sampleRate == 0) {
            c.sampleRate = 48000;
        }
    }

    engine = (ma_engine *)calloc(1, sizeof(*engine));
    if (engine == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_engine_init(&c, engine);
    if (result != MA_SUCCESS) {
        free(engine);
        return result;
    }

    *out = (eco_ma_engine *)engine;
    return MA_SUCCESS;
}

void eco_ma_engine_uninit(eco_ma_engine *engine)
{
    ma_engine *inner;

    if (engine == NULL) {
        return;
    }

    inner = as_engine(engine);
    ma_engine_uninit(inner);
    free(inner);
}

void eco_ma_engine_listener_get_position(eco_ma_engine *engine, uint32_t index, eco_ma_vec3 *out)
{
    if (engine == NULL) {
        return;
    }

    store_xyz(out, ma_engine_listener_get_position(as_engine(engine), index));
}

void eco_ma_engine_listener_get_direction(eco_ma_engine *engine, uint32_t index, eco_ma_vec3 *out)
{
    if (engine == NULL) {
        return;
    }

    store_xyz(out, ma_engine_listener_get_direction(as_engine(engine), index));
}

void eco_ma_engine_listener_get_world_up(eco_ma_engine *engine, uint32_t index, eco_ma_vec3 *out)
{
    if (engine == NULL) {
        return;
    }

    store_xyz(out, ma_engine_listener_get_world_up(as_engine(engine), index));
}
