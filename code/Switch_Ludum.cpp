#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dirent.h>

#include <switch.h>

#include "Ludum.h"
#include "Ludum.cpp"

Platform_Context Platform;

RENDERER_INITIALISE(SwitchOpenGLInitialise);

#define SDL_MAIN_HANDLED 1
#include <SDL2/SDL.h>

global SDL_AudioDeviceID global_audio_device;

global int global_nxlink_socket = -1;

struct Switch_Audio_State {
    b32 enabled;

    AudioDriver driver;
    b32 running;

    u32 max_sample_count;
    u32 samples_queued;

    s16 *sample_buffer;
    u32 write_cursor;

    AudioDriverWaveBuf buffers[2];

    Thread thread;
    Mutex mutex;
};

struct Switch_File_List {
    Memory_Allocator alloc;
};

internal PLATFORM_ALLOCATE(SwitchAllocate) {
    Memory_Block *result = 0;

    umm full_size = size + sizeof(Memory_Block);
    result = cast(Memory_Block *) malloc(full_size);

    result->base  = cast(u8 *) (result + 1);
    result->size  = size;
    result->used  = 0;
    result->flags = flags;
    result->prev  = 0;

    return result;
}

internal PLATFORM_DEALLOCATE(SwitchDeallocate) {
    // :MemoryFlags
    //
    free(block);
}

internal PLATFORM_FILE_ERROR(SwitchFileError) {
    printf("File error: %s\n", message);

    handle->errors = true;
}

internal File_Info *SwitchAllocateFileInfo(File_List *list, struct stat *info, string basename) {
    File_Info *result;

    Switch_File_List *switch_list = cast(Switch_File_List *) list->platform;
    result = AllocStruct(&switch_list->alloc, File_Info);

    result->basename  = basename;
    result->timestamp = info->st_mtim.tv_sec + (1000000000 * info->st_mtim.tv_nsec);
    result->size      = info->st_size;

    result->next = list->first;
    list->first = result;

    list->file_count += 1;

    return result;
}

internal PLATFORM_GET_FILE_LIST(SwitchGetFileList) {
    File_List result = {};

    Switch_File_List *switch_list = InlineAlloc(Switch_File_List, alloc);
    result.platform = cast(void *) switch_list;

    const char *dir_name = 0;
    string ending;
    switch (type) {
        case FileType_Amt: {
            dir_name = "data/";
            ending = WrapZ(".amt");
        }
        break;
        case FileType_Config: {} break;
    }

    DIR *dir = opendir(dir_name);
    struct dirent *entry;
    while ((entry = readdir(dir)) != 0) {
        if (entry->d_type != DT_REG) { continue; }

        string filename = WrapZ(entry->d_name);
        if (!EndsWith(filename, ending)) { continue; }

        char buf[1024];

        string full_path;
        full_path.count = snprintf(buf, sizeof(buf), "%s%.*s", dir_name, cast(u32) filename.count, filename.data) + 1;
        full_path.data  = cast(u8 *) buf;

        struct stat file_stat;
        if (stat(buf, &file_stat) != 0) { continue; }

        string basename = filename;
        for (umm it = 0; it < filename.count; ++it) {
            if (filename.data[it] == '.') {
                basename.count = it;
            }
        }

        File_Info *info = SwitchAllocateFileInfo(&result, &file_stat, CopyString(&switch_list->alloc, basename));
        info->platform  = CopyString(&switch_list->alloc, full_path).data;
    }

    return result;
}

internal PLATFORM_CLEAR_FILE_LIST(SwitchClearFileList) {
    Switch_File_List *switch_list = cast(Switch_File_List *) list->platform;
    if (switch_list) {
        Clear(&switch_list->alloc);
    }
}

internal PLATFORM_OPEN_FILE(SwitchOpenFile) {
    File_Handle result = {};

    char mode[3] = { 0, 0, 0 };
    if (access & FileAccess_Read) {
        mode[0] = 'r';
    }

    if (access & FileAccess_Write) {
        if (mode[0] == 'r') { mode[1] = 'w'; }
        else { mode[0] = 'w'; }
    }

    const char *filename = cast(const char *) info->platform;
    FILE *file = fopen(filename, mode);
    if (!file) {
        SwitchFileError(&result, "Failed to open file");
    }
    else {
        result.platform = cast(void *) file;
    }

    return result;
}

internal PLATFORM_CLOSE_FILE(SwitchCloseFile) {
    FILE *file = cast(FILE *) handle->platform;
    fclose(file);
}

internal PLATFORM_READ_FILE(SwitchReadFile) {
    if (!IsValid(handle)) { return; }

    FILE *file = cast(FILE *) handle->platform;

    fseek(file, offset, SEEK_SET);
    umm count = fread(data, size, 1, file);
    if (count != size) {
        //SwitchFileError(handle, "Incomplete read on file");
    }
}

internal PLATFORM_WRITE_FILE(SwitchWriteFile) {
    if (!IsValid(handle)) { return; }

    FILE *file = cast(FILE *) handle->platform;

    fseek(file, offset, SEEK_SET);
    umm count = fwrite(data, size, 1, file);
    if (count != size) {
        SwitchFileError(handle, "Incomplete write on file");
    }
}

internal v2u SwitchGetWindowSize() {
    v2u result;
    nwindowGetDimensions(nwindowGetDefault(), &result.w, &result.h);

    return result;
}

internal void SwitchHandleButtonPress(Game_Button *button, b32 pressed) {
    if (button->pressed != pressed) {
        button->pressed       = pressed;
        button->transistions += 1;
    }
}

internal void SwitchHandleInput(PadState *pad, Game_Input *current_input, Game_Input *prev_input) {
    padUpdate(pad);

    u64 buttons = padGetButtons(pad);

    Game_Controller *current = GameGetController(current_input, 0);
    Game_Controller *prev    = GameGetController(prev_input,    0);

    for (u32 it = 0; it < ArrayCount(Game_Controller::buttons); ++it) {
        current->buttons[it].pressed      = prev->buttons[it].pressed;
        current->buttons[it].transistions = 0;
    }

    // Directional buttons
    //
    b32 pressed = (buttons & HidNpadButton_StickLLeft) || (buttons & HidNpadButton_Left);
    SwitchHandleButtonPress(&current->left, pressed);
    pressed = (buttons & HidNpadButton_StickLRight) || (buttons & HidNpadButton_Right);
    SwitchHandleButtonPress(&current->right, pressed);
    pressed = (buttons & HidNpadButton_StickLUp) || (buttons & HidNpadButton_Up);
    SwitchHandleButtonPress(&current->up, pressed);
    pressed = (buttons & HidNpadButton_StickLDown) || (buttons & HidNpadButton_Down);
    SwitchHandleButtonPress(&current->down, pressed);

    // Action & interact
    //
    pressed = (buttons & HidNpadButton_A);
    SwitchHandleButtonPress(&current->action, pressed);
    pressed = (buttons & HidNpadButton_X);
    SwitchHandleButtonPress(&current->interact, pressed);

    // Menu
    //
    pressed = (buttons & HidNpadButton_Plus);
    SwitchHandleButtonPress(&current->menu, pressed);

    pressed = (buttons & HidNpadButton_Minus);
    if (pressed) { current_input->requested_quit = true; }

    current_input->mouse_clip = prev_input->mouse_clip;

    HidTouchScreenState touch_state = {};
    if (hidGetTouchScreenStates(&touch_state, 1)) {
        if (touch_state.count != 0) {
            u32 index = touch_state.count - 1;

            f32 x = cast(f32) touch_state.touches[index].x;
            f32 y = cast(f32) touch_state.touches[index].y;

            v2 window_size = V2(SwitchGetWindowSize());

            x = -1.0f + ((2.0f * x) / window_size.x);
            y =  1.0f - ((2.0f * y) / window_size.y);

            current_input->mouse_clip = V3(x, y, 0);
        }
    }
}

internal u64 SwitchGetCurrentTicks() {
    u64 result;

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    result = ((current_time.tv_sec * 1000000000) + current_time.tv_nsec);
    return result;
}

internal f32 SwitchGetElapsedTime(u64 start, u64 end) {
    f32 result = cast(f32) (end - start) / 1000000000.0f;
    return result;
}

internal AudioDriverWaveBuf *SwitchGetFreeAudioBuffer(Switch_Audio_State *audio_state) {
    AudioDriverWaveBuf *result = 0;

    for (u32 it = 0; it < ArrayCount(audio_state->buffers); ++it) {
        AudioDriverWaveBuf *buffer = &audio_state->buffers[it];
        if (buffer->state == AudioDriverWaveBufState_Free || buffer->state == AudioDriverWaveBufState_Done) {
            result = buffer;
            break;
        }
    }

    return result;
}

internal void SwitchOutputSounds(Switch_Audio_State *audio_state, Game_Context *context, Sound_Buffer *sound_buffer) {
    u32 samples_queued = __atomic_load_n(&audio_state->samples_queued, __ATOMIC_SEQ_CST);

    if (samples_queued < audio_state->max_sample_count) {
        u32 sample_count = audio_state->max_sample_count - samples_queued;

        sound_buffer->sample_count = sample_count;
        sound_buffer->samples      = audio_state->sample_buffer + audio_state->write_cursor;

        __atomic_add_fetch(&audio_state->write_cursor, sample_count, __ATOMIC_SEQ_CST);

        mutexLock(&audio_state->mutex);
        LudumOutputSoundSamples(context, sound_buffer);
        mutexUnlock(&audio_state->mutex);

        __atomic_add_fetch(&audio_state->samples_queued, sample_count, __ATOMIC_SEQ_CST);
    }
}

internal void SwitchAudioThreadProc(void *param) {
    Switch_Audio_State *audio_state = cast(Switch_Audio_State *) param;
    while (true) {
        if (audio_state->samples_queued <= 0) {
            audrvUpdate(&audio_state->driver);
            audrenWaitFrame();
            continue;
        }

        printf("BLAHHHHHH..\n");

        AudioDriverWaveBuf *wave_buffer = SwitchGetFreeAudioBuffer(audio_state);
        if (wave_buffer) {
            void *output_buffer = cast(void *) (wave_buffer->data_pcm16 + wave_buffer->start_sample_offset);
            u32 sample_count    = __atomic_load_n(&audio_state->samples_queued, __ATOMIC_SEQ_CST);

            printf("Copying %d samples... ", sample_count);

            mutexLock(&audio_state->mutex);
            CopySize(output_buffer, audio_state->sample_buffer, sample_count * sizeof(s16));
            mutexUnlock(&audio_state->mutex);

            printf("Finished Copy!\n");

            __atomic_store_n(&audio_state->write_cursor, 0, __ATOMIC_SEQ_CST);
            __atomic_store_n(&audio_state->samples_queued, 0, __ATOMIC_SEQ_CST);

            wave_buffer->end_sample_offset = wave_buffer->start_sample_offset + sample_count;

            audrvVoiceAddWaveBuf(&audio_state->driver, 0, wave_buffer);

            audrvUpdate(&audio_state->driver);

            s32 samples_played = 0;
            while (wave_buffer->state == AudioDriverWaveBufState_Playing) {
                //printf("Playing count = %d\n", samples_played);
                //samples_played = (cast(s32) audrvVoiceGetPlayedSampleCount(&audio_state->driver, 0) - samples_played);
                //__atomic_sub_fetch(&audio_state->samples_queued, samples_played, __ATOMIC_SEQ_CST);

                audrvUpdate(&audio_state->driver);
                audrenWaitFrame();
            }
        }
        else {
            wave_buffer = 0;
            for (u32 it = 0; it < ArrayCount(audio_state->buffers); ++it) {
                if (audio_state->buffers[it].state == AudioDriverWaveBufState_Playing) {
                    wave_buffer = &audio_state->buffers[it];
                    break;
                }
            }

            while (wave_buffer->state == AudioDriverWaveBufState_Playing) {
                audrvUpdate(&audio_state->driver);
                audrenWaitFrame();
            }

            continue;
        }
    }
}

internal b32 SwitchInitialiseAudio(Switch_Audio_State *audio_state) {
    b32 result = false;

    u32 sample_rate   = 48000;
    u32 channel_count = 2;

    AudioRendererConfig config = {};
    config.output_rate     = AudioRendererOutputRate_48kHz;
    config.num_voices      = 24;
    config.num_effects     = 0;
    config.num_sinks       = 1;
    config.num_mix_objs    = 1;
    config.num_mix_buffers = 2;

    Result rc = audrenInitialize(&config);
    if (R_FAILED(rc)) {
        printf("Failed to initialise audio renderer\n");
        return result;
    }

    rc = audrvCreate(&audio_state->driver, &config, 2);
    if (R_FAILED(rc)) {
        printf("Failed to create driver\n");
        return result;
    }

    audio_state->max_sample_count = sample_rate / 4; // * 10;

    umm mem_pool_size = 2 * (audio_state->max_sample_count * channel_count * sizeof(s16));
    mem_pool_size     = (mem_pool_size + 0xFFF) & ~0xFFF;

    void *mem_pool_base = memalign(0x1000, mem_pool_size);
    s32 mem_pool_id = audrvMemPoolAdd(&audio_state->driver, mem_pool_base, mem_pool_size);
    audrvMemPoolAttach(&audio_state->driver, mem_pool_id);

    ZeroSize(mem_pool_base, mem_pool_size);
    armDCacheFlush(mem_pool_base, mem_pool_size);

    u8 channel_ids[2] = { 0, 1 };
    audrvDeviceSinkAdd(&audio_state->driver, AUDREN_DEFAULT_DEVICE_NAME, 2, channel_ids);

    audrvUpdate(&audio_state->driver);

    audrenStartAudioRenderer();

    audrvVoiceInit(&audio_state->driver, 0, channel_count, PcmFormat_Int16, sample_rate);

    audrvVoiceSetDestinationMix(&audio_state->driver, 0, AUDREN_FINAL_MIX_ID);

    audrvVoiceSetMixFactor(&audio_state->driver, 0, 1.0f, 0, 0);
    audrvVoiceSetMixFactor(&audio_state->driver, 0, 0.0f, 0, 1);

    audrvVoiceSetMixFactor(&audio_state->driver, 0, 0.0f, 1, 0);
    audrvVoiceSetMixFactor(&audio_state->driver, 0, 1.0f, 1, 1);

    for (u32 it = 0; it < ArrayCount(audio_state->buffers); ++it) {
        AudioDriverWaveBuf *buffer  = &audio_state->buffers[it];

        buffer->state               = AudioDriverWaveBufState_Free;
        buffer->data_raw            = mem_pool_base;
        buffer->size                = mem_pool_size;
        buffer->start_sample_offset = (it * audio_state->max_sample_count);
        buffer->end_sample_offset   = 0;
        buffer->is_looping          = false;
    }

    audio_state->sample_buffer = cast(s16 *) malloc(audio_state->max_sample_count * sizeof(s16));

    audrvVoiceStart(&audio_state->driver, 0);
    audrvUpdate(&audio_state->driver);

    printf("CREATING MUTEX\n");
    mutexInit(&audio_state->mutex);

    printf("CREATING THREAD\n");

    rc = threadCreate(&audio_state->thread, SwitchAudioThreadProc, cast(void *) audio_state, 0, Megabytes(8), 0x3B, -2);
    if (R_FAILED(rc)) {
        printf("Failed to create audio thread (0x%x)\n", rc);
        return result;
    }

    printf("STARTING THREAD\n");

    threadStart(&audio_state->thread);

    printf("STARTED THREAD\n");

#if 0
    rc = threadStart(&audio_state->thread);
    if (R_FAILED(rc)) {
        printf("Failed to start audio thread (0x%x)\n", rc);
        return result;
    }
#endif

#if 0
    umm mem_pool_size = 2 * (audio_state->max_sample_count * channel_count * sizeof(s16));
    mem_pool_size = (mem_pool_size + 0xFFF) & ~0xFFF;

    void *mem_pool_base = memalign(0x1000, mem_pool_size);
    s32 mem_pool_id = audrvMemPoolAdd(&audio_state->driver, mem_pool_base, mem_pool_size);
    audrvMemPoolAttach(&audio_state->driver, mem_pool_id);

    ZeroSize(mem_pool_base, mem_pool_size);
    armDCacheFlush(mem_pool_base, mem_pool_size);

    u8 channel_ids[2] = { 0, 1 };
    audrvDeviceSinkAdd(&audio_state->driver, AUDREN_DEFAULT_DEVICE_NAME, 2, channel_ids);

    audrvUpdate(&audio_state->driver);

    audrenStartAudioRenderer();

    printf("Started audio renderer\n");

    for (u32 it = 0; it < 3; ++it) {
        audrvVoiceInit(&audio_state->driver, it, channel_count, PcmFormat_Int16, sample_rate);

        audrvVoiceSetDestinationMix(&audio_state->driver, it, AUDREN_FINAL_MIX_ID);

        audrvVoiceSetMixFactor(&audio_state->driver, it, 1.0f, 0, 0);
        audrvVoiceSetMixFactor(&audio_state->driver, it, 0.0f, 1, 0);
        audrvVoiceSetMixFactor(&audio_state->driver, it, 0.0f, 0, 1);
        audrvVoiceSetMixFactor(&audio_state->driver, it, 1.0f, 1, 1);
    }

    printf("Setup voices\n");

    for (u32 it = 0; it < ArrayCount(audio_state->buffers); ++it) {
        AudioDriverWaveBuf *buffer = &audio_state->buffers[it];

        buffer->state               = AudioDriverWaveBufState_Free;
        buffer->data_raw            = mem_pool_base;
        buffer->size                = mem_pool_size;
        buffer->start_sample_offset = (it * audio_state->max_sample_count);
        buffer->end_sample_offset   = buffer->start_sample_offset;
    }

    printf("Started voices\n");

    audio_state->current_buffer_index = -1;
#endif

    result = true;
    return result;
}

#if 0
internal void SwitchOutputSounds(Switch_Audio_State *audio_state, Game_Context *context, Sound_Buffer *sound_buffer) {
    if (audrvVoiceIsPlaying(&audio_state->driver, 0)) {
        audio_state->read_cursor += audrvVoiceGetPlayedSampleCount(&audio_state->driver, 0);
    }

    printf("-- Read cursor: %d, Write cursor: %d --\n", audio_state->read_cursor, audio_state->write_cursor);

    s32 samples_to_write = audio_state->max_sample_count - (audio_state->write_cursor - audio_state->read_cursor);
    if (samples_to_write > 0) {
        sound_buffer->sample_count = samples_to_write;
        sound_buffer->samples      = audio_state->sample_buffer;

        printf("Getting samples from game\n");

        LudumOutputSoundSamples(context, sound_buffer);

        if ((audio_state->sample_index + samples_to_write) > audio_state->max_sample_count) {
            printf("Split output of %d samples\n", samples_to_write);
            // Segment one
            //
            AudioDriverWaveBuf *buffer_0 = SwitchGetFreeAudioBuffer(audio_state);
            if (buffer_0) {
                buffer_0->start_sample_offset = audio_state->sample_index;
                buffer_0->end_sample_offset   = audio_state->max_sample_count;

                s16 *samples_to   = buffer_0->data_pcm16 + buffer_0->start_sample_offset;
                s16 *samples_from = sound_buffer->samples;
                u32 sample_count  = (buffer_0->end_sample_offset - buffer_0->start_sample_offset);

                CopySize(samples_to, samples_from, sample_count * sizeof(s16));
                armDCacheFlush(samples_to, sample_count * sizeof(s16));

                audrvVoiceAddWaveBuf(&audio_state->driver, 0, buffer_0);
            }

            // Segment two
            //
            u32 sample_offset = (audio_state->max_sample_count - audio_state->sample_index);

            AudioDriverWaveBuf *buffer_1  = SwitchGetFreeAudioBuffer(audio_state);
            if (buffer_1) {
                buffer_1->start_sample_offset = 0;
                buffer_1->end_sample_offset   = samples_to_write - sample_offset;

                s16 *samples_to   = buffer_1->data_pcm16;
                s16 *samples_from = sound_buffer->samples + sample_offset;
                u32 sample_count  = buffer_1->end_sample_offset;

                CopySize(samples_to, samples_from, sample_count * sizeof(s16));
                armDCacheFlush(samples_to, sample_count * sizeof(s16));

                audrvVoiceAddWaveBuf(&audio_state->driver, 0, buffer_1);
            }

            audio_state->sample_index = samples_to_write - sample_offset;
        }
        else {
            printf("Outputting %d samples\n", samples_to_write);
            // Segment one
            //
            AudioDriverWaveBuf *buffer_0 = SwitchGetFreeAudioBuffer(audio_state);
            if (buffer_0) {
                buffer_0->start_sample_offset = audio_state->sample_index;
                buffer_0->end_sample_offset   = audio_state->sample_index + samples_to_write;

                s16 *samples_to   = buffer_0->data_pcm16 + audio_state->sample_index;
                s16 *samples_from = sound_buffer->samples;
                u32 sample_count  = samples_to_write;

                //printf("Copying samples!\n");

                CopySize(samples_to, samples_from, sample_count * sizeof(s16));
                armDCacheFlush(samples_to, sample_count * sizeof(s16));

               // printf("Queuing buffer!\n");
                // Queue the buffer
                //
                audrvVoiceAddWaveBuf(&audio_state->driver, 0, buffer_0);
            }

            audio_state->sample_index += samples_to_write;
            audio_state->sample_index %= audio_state->max_sample_count;

        }

        audio_state->write_cursor += samples_to_write;
    }

    printf("Updating... ");
//    audrvUpdate(&audio_state->driver);
    printf("Finished!\n\n");
#if 0
    AudioDriverWaveBuf *buffer = audio_state->current_buffer;
    if (!buffer) {
        buffer = SwitchGetFreeAudioBuffer(audio_state);
        if (!buffer) {
            printf("Warning: No audio buffer free\n");
            audrvUpdate(&audio_state->driver);
            return;
        }

        audio_state->current_buffer  = buffer;
        // audio_state->samples_written = 0;
    }




    if (samples_to_write > 0) {
        samples_to_write = Min(cast(u32) samples_to_write, audio_state->max_sample_count - audio_state->samples_written);

        sound_buffer->sample_count = samples_to_write;
        sound_buffer->samples      = audio_state->sample_buffer;

        LudumOutputSoundSamples(context, sound_buffer);

        // This copy is dumb
        //
        s16 *output_samples = cast(s16 *) buffer->data_raw;
        u32 sample_offset   = buffer->start_sample_offset + (audio_state->write_cursor % audio_state->max_sample_count);
        for (u32 it = 0; it < sound_buffer->sample_count; ++it) {
            u32 sample_index = (sample_offset + it);

            output_samples[(sample_index + 0) % audio_state->max_sample_count] = sound_buffer->samples[it];
        }

        armDCacheFlush(output_samples, sound_buffer->sample_count * sizeof(s16));

        audio_state->write_cursor    += sound_buffer->sample_count;
        audio_state->samples_written += sound_buffer->sample_count;

        if (audio_state->samples_written >= audio_state->max_sample_count) {
            audrvVoiceStop(&audio_state->driver, 0);
            audrvVoiceAddWaveBuf(&audio_state->driver, 0, buffer);
            audrvVoiceStart(&audio_state->driver, 0);

            audio_state->current_buffer  = 0;
            audio_state->samples_written = 0;
        }
    }
#endif

#if 0
    if (buffer->state == AudioDriverWaveBufState_Free || buffer->state == AudioDriverWaveBufState_Done) {
        audrvVoiceStop(&audio_state->driver, 0);
        audrvVoiceAddWaveBuf(&audio_state->driver, 0, buffer);
        audrvVoiceStart(&audio_state->driver, 0);
    }
#endif

#if 0
    AudioDriverWaveBuf *buffer = 0;
    s32 index = audio_state->current_buffer_index;

    if (index < 0) {
        index = SwitchGetFreeAudioBuffer(audio_state);
        if (index < 0) {
            audrvUpdate(&audio_state->driver);
            return;
        }

        printf("Using buffer index %d\n", index);

        audio_state->current_buffer_index = index;

        buffer = &audio_state->buffers[index];
        buffer->end_sample_offset = buffer->start_sample_offset;

        audrvVoiceStop(&audio_state->driver, 0);
    }
    else {
        buffer = &audio_state->buffers[index];
    }

    s32 other_index = (index == 0) ? 1 : 0;

    u32 samples_played = 0;
    if (audrvVoiceIsPlaying(&audio_state->driver, other_index)) {
        samples_played = audrvVoiceGetPlayedSampleCount(&audio_state->driver, other_index);
    }
    else {
        samples_played = audio_state->max_sample_count;
    }

    u32 samples_this_frame = samples_played; //(1000 / 60);
    u32 current_samples = SwitchGetAudioBufferSampleCount(buffer);
    if ((samples_this_frame + current_samples) > audio_state->max_sample_count) {
        samples_this_frame = audio_state->max_sample_count - current_samples;
    }

    sound_buffer->sample_count = samples_this_frame;
    sound_buffer->samples      = cast(s16 *) buffer->data_raw + buffer->end_sample_offset;
    LudumOutputSoundSamples(context, sound_buffer);

    buffer->end_sample_offset += samples_this_frame;

    current_samples += samples_this_frame;
    if (current_samples >= audio_state->max_sample_count) {
        void *samples = cast(void *) (cast(u8 *) buffer->data_raw + buffer->start_sample_offset);
        armDCacheFlush(samples, audio_state->max_sample_count);

        audrvVoiceAddWaveBuf(&audio_state->driver, 0, buffer);
        audrvVoiceStart(&audio_state->driver, 0);

        audio_state->current_buffer_index = -1;
    }

    audrvUpdate(&audio_state->driver);
#endif
}
#endif

internal void SDL2OutputSounds(Game_Context *context, Sound_Buffer *sound_buffer) {
    const u32 max_sample_count = 12000;
    umm frame_sample_size = (max_sample_count * sizeof(s16)) - SDL_GetQueuedAudioSize(global_audio_device);

    sound_buffer->sample_count = frame_sample_size / sizeof(s16);
    LudumOutputSoundSamples(context, sound_buffer);

    if (global_audio_device != 0) {
        SDL_QueueAudio(global_audio_device, sound_buffer->samples, frame_sample_size);
    }
}

internal b32 SwitchInitialise() {
    b32 result = false;

    Result rc = socketInitializeDefault();
    if (R_SUCCEEDED(rc)) {
        global_nxlink_socket = nxlinkStdio();
        if (global_nxlink_socket >= 0) {
            printf("Connected to nxlink\n");
        }
        else {
            socketExit();
        }
    }

    rc = romfsInit();
    if (R_FAILED(rc)) {
        return result;
    }

    chdir("romfs://");

    Platform.Allocate      = SwitchAllocate;
    Platform.Deallocate    = SwitchDeallocate;

    Platform.FileError     = SwitchFileError;

    Platform.GetFileList   = SwitchGetFileList;
    Platform.ClearFileList = SwitchClearFileList;

    Platform.OpenFile      = SwitchOpenFile;
    Platform.CloseFile     = SwitchCloseFile;

    Platform.ReadFile      = SwitchReadFile;
    Platform.WriteFile     = SwitchWriteFile;

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    result = true;
    return result;
}

int main(int argc, char **argv) {
    if (!SwitchInitialise()) {
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("Failed to init audio\n");
        return 1;
    }

    SDL_AudioSpec desired;
    SDL_AudioSpec acquired;

    desired.freq     = 48000;
    desired.format   = AUDIO_S16LSB;
    desired.channels = 2;
    desired.samples  = 1024;
    desired.callback = 0;
    desired.userdata = 0;

    global_audio_device = SDL_OpenAudioDevice(0, 0, &desired, &acquired, 0);
    if (global_audio_device != 0) {
        SDL_PauseAudioDevice(global_audio_device, 0);
    }
    else {
        printf("Couldn't get audio device\n");
    }

    Renderer_Parameters params = {};
    params.command_buffer_size          = Megabytes(1);
    params.max_immediate_vertex_count   = (1 << 17);
    params.max_immediate_index_count    = (1 << 18);
    params.max_texture_handle_count     = 128;
    params.texture_transfer_buffer_size = Megabytes(128);
    params.platform_data                = cast(void *) nwindowGetDefault();

    Renderer_Context *renderer = SwitchOpenGLInitialise(&params);
    if (!IsValid(renderer)) {
        renderer->Shutdown(renderer);

        if (global_nxlink_socket >= 0) {
            close(global_nxlink_socket);
            socketExit();
            global_nxlink_socket = -1;
        }

        return 1;
    }

    PadState pad;
    padInitializeDefault(&pad);

    Game_Input inputs[2] = {};
    Game_Input *current_input = &inputs[0];
    Game_Input *prev_input    = &inputs[1];

    GameGetController(current_input, 0)->connected = true;
    GameGetController(prev_input,    0)->connected = true;

    Game_Context _context = {};
    Game_Context *context = &_context;

    context->state          = 0;
    context->texture_buffer = &renderer->texture_buffer;

    Sound_Buffer sound_buffer = {};
    sound_buffer.sample_count = 12000;
    sound_buffer.samples      = cast(s16 *) malloc(sound_buffer.sample_count * sizeof(s16));

#if 0
    Switch_Audio_State audio_state = {};
    if (!SwitchInitialiseAudio(&audio_state)) {
        printf("[Warning] :: Audio disabled. Failed to initialise\n");
        return 1;
    }
#endif

    b32 running = true;

    u64 start = SwitchGetCurrentTicks();
    while (running) {
        {
            u64 end = SwitchGetCurrentTicks();
            current_input->delta_time = SwitchGetElapsedTime(start, end);

            start = end;
        }

        SwitchHandleInput(&pad, current_input, prev_input);
        if (current_input->requested_quit) { running = false; }

        rect2 render_region;
        v2u window_size = SwitchGetWindowSize();

        v2 render_size;
        render_size.x = 4 * (window_size.y / 3);
        render_size.y = window_size.y;

        render_region.min = (0.5 * (V2(window_size) - render_size));
        render_region.max = render_region.min + render_size;

        Draw_Command_Buffer *command_buffer = renderer->BeginFrame(renderer, render_region);

        // @Todo(James): Fix clock. This doesn't have any sort of time synchronisation
        //
        LudumUpdateRender(context, current_input, command_buffer);
        if (current_input->requested_quit) { running = false; }

        renderer->EndFrame(renderer, command_buffer);

        SDL2OutputSounds(context, &sound_buffer);

        Swap(current_input, prev_input);
    }

    renderer->Shutdown(renderer);

#if 0
    threadClose(&audio_state.thread);

    audrvClose(&audio_state.driver);
    audrenExit();
#endif

    SDL_CloseAudioDevice(global_audio_device);
    SDL_Quit();

    if (global_nxlink_socket >= 0) {
        close(global_nxlink_socket);
        socketExit();
        global_nxlink_socket = -1;
    }

    return 0;
}
