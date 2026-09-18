#include "internal.h"

#include <stdlib.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

static void eco_ma_drop_ios_playback_context(ma_context *ctx)
{
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (ctx == NULL) {
        return;
    }

    ma_context_uninit(ctx);
    free(ctx);
#else
    (void)ctx;
#endif
}

/*
 * ma_engine_init always passes a non-NULL context config, so miniaudio's
 * playback-device -> Playback-category hack does not run and the default
 * is PlayAndRecord (microphone). Own a Playback context on iOS instead.
 */
static ma_result eco_ma_attach_ios_playback_context(ma_engine_config *c, ma_context **out)
{
    *out = NULL;

#if defined(__APPLE__) && TARGET_OS_IPHONE
    {
        ma_context *ctx;
        ma_context_config ctx_cfg;
        ma_result result;

        if (c->noDevice) {
            return MA_SUCCESS;
        }

        ctx = (ma_context *)calloc(1, sizeof(*ctx));
        if (ctx == NULL) {
            return MA_OUT_OF_MEMORY;
        }

        ctx_cfg = ma_context_config_init();
        ctx_cfg.coreaudio.sessionCategory = ma_ios_session_category_playback;
        result = ma_context_init(NULL, 0, &ctx_cfg, ctx);
        if (result != MA_SUCCESS) {
            free(ctx);
            return result;
        }

        c->pContext = ctx;
        *out = ctx;
        return MA_SUCCESS;
    }
#else
    (void)c;
    return MA_SUCCESS;
#endif
}

int32_t eco_ma_engine_init(
    uint32_t channels,
    uint32_t sample_rate,
    uint32_t no_auto_start,
    uint32_t no_device,
    eco_ma_engine **out
)
{
    ma_engine_config c;
    eco_ma_engine *box;
    ma_context *ctx = NULL;
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

    box = (eco_ma_engine *)calloc(1, sizeof(*box));
    if (box == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = eco_ma_attach_ios_playback_context(&c, &ctx);
    if (result != MA_SUCCESS) {
        free(box);
        return result;
    }

    result = ma_engine_init(&c, as_engine(box));
    if (result != MA_SUCCESS) {
        eco_ma_drop_ios_playback_context(ctx);
        free(box);
        return result;
    }

    box->owned_context = ctx;
    *out = box;
    return MA_SUCCESS;
}

void eco_ma_engine_uninit(eco_ma_engine *engine)
{
    if (engine == NULL) {
        return;
    }

    ma_engine_uninit(as_engine(engine));
    eco_ma_drop_ios_playback_context(engine->owned_context);
    engine->owned_context = NULL;
    free(engine);
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
