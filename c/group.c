#include "internal.h"

#include <stdlib.h>

int32_t eco_ma_sound_group_init(eco_ma_engine *engine, eco_ma_sound_group **out)
{
    ma_sound_group *group;
    ma_result result;

    if (engine == NULL || out == NULL) {
        return MA_INVALID_ARGS;
    }

    *out = NULL;
    group = (ma_sound_group *)calloc(1, sizeof(*group));
    if (group == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_sound_group_init(as_engine(engine), 0, NULL, group);
    if (result != MA_SUCCESS) {
        free(group);
        return result;
    }

    *out = (eco_ma_sound_group *)group;
    return MA_SUCCESS;
}

void eco_ma_sound_group_uninit(eco_ma_sound_group *group)
{
    ma_sound_group *inner;

    if (group == NULL) {
        return;
    }

    inner = as_group(group);
    ma_sound_group_uninit(inner);
    free(inner);
}
