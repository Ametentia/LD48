#include "Ludum_Assets.cpp"
#include "Ludum_Renderer.cpp"

#include "Ludum_Mode_Play.cpp"
#include "Ludum_Mode_Splash.cpp"

internal void LudumUpdateRender(Game_Context *context, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Game_State *state = context->state;
    if (!state) {
        state = InlineAlloc(Game_State, perm);
        context->state = state;

        state->temp = AllocStruct(&state->perm, Memory_Allocator);
        state->temp_handle = BeginTemp(state->temp);

        InitialiseAssetManager(state, &state->assets, context->texture_buffer);

        state->master_volume = 1.0;
    }

    EndTemp(state->temp_handle);
    state->temp_handle = BeginTemp(state->temp);

    if (state->mode == GameMode_None) {
        //ModePlay(state);
        ModeSplash(state);
    }

    switch (state->mode) {
        case GameMode_Play: {
            UpdateRenderModePlay(state, input, draw_buffer);
        }
        break;
        case GameMode_Splash: {
            UpdateRenderModeSplash(state, input, draw_buffer);
        }
        break;
    }
}

internal void LudumOutputSoundSamples(Game_Context *context, Sound_Buffer *buffer) {
    Game_State *state = context->state;

    Temporary_Memory mixer_temp = BeginTemp(state->temp);

    // Mix in 32-bit float space
    //
    f32 *mixer_samples = AllocArray(state->temp, f32, buffer->sample_count);

    for (Playing_Sound **sound_ptr = &state->playing_sounds; *sound_ptr != 0;) {
        Playing_Sound *sound = *sound_ptr;

        Amt_Sound *sound_info = GetSoundInfo(&state->assets, sound->handle);
        s16 *sound_samples    = GetSoundData(&state->assets, sound->handle);
        if (!sound_samples) {
            *sound_ptr = sound->next;

            sound->next = state->free_playing_sounds;
            state->free_playing_sounds = sound;

            continue;
        }

        u32 sample_offset = sound->samples_played;
        u32 sample_count  = buffer->sample_count;
        if ((sample_offset + sample_count) > sound_info->sample_count) {
            sample_count = sound_info->sample_count - sound->samples_played;
        }

        s16 *samples = &sound_samples[sample_offset];
        for (u32 it = 0; it < sample_count; ++it) {
            mixer_samples[it] += (state->master_volume * sound->volume * samples[it]);
        }

        sound->samples_played += sample_count;
        if (sound->samples_played >= sound_info->sample_count) {
            if (sound->flags & PlayingSound_Looped) {
                sound->samples_played = 0;
            }
            else {
                *sound_ptr = sound->next;
                sound->next = state->free_playing_sounds;
                state->free_playing_sounds = sound;
            }
        }
        else {
            sound_ptr = &sound->next;
        }

    }

    // Convert back to signed 16-bit
    //
    for (u32 it = 0; it < buffer->sample_count; ++it) {
        buffer->samples[it] = cast(s16) (mixer_samples[it] + 0.5f);
    }

    EndTemp(mixer_temp);
}
