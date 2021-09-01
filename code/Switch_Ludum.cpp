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

global int global_nxlink_socket = -1;

struct Switch_Audio_State {
    b32 enabled;
    b32 running;

    s32 mem_pool_id;
    s32 sink;

    AudioDriver driver;

    u32 write_cursor;
    u32 read_cursor;

    u32 max_sample_count;
    u32 samples_queued;
    s16 *sample_queue;

    u32 buffer_sample_count; // Number of samples dequeued per buffer
    AudioDriverWaveBuf buffers[2];

    Thread thread;
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
        pressed = touch_state.count != 0;
        SwitchHandleButtonPress(&current_input->mouse_buttons[0], pressed);

        if (pressed) {
            u32 index = touch_state.count - 1;

            f32 x = cast(f32) touch_state.touches[index].x;
            f32 y = cast(f32) touch_state.touches[index].y;

            v2 window_size = V2(SwitchGetWindowSize());

            x = -1.0f + ((2.0f * x) / window_size.x);
            y =  1.0f - ((2.0f * y) / window_size.y);

            v3 old_mouse = current_input->mouse_clip;

            current_input->mouse_clip  = V3(x, y, 0);
            current_input->mouse_delta = current_input->mouse_clip - old_mouse;
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

internal s16 *SwitchGetAudioBufferBase(AudioDriverWaveBuf *buffer) {
    s16 *result = cast(s16 *) &buffer->data_pcm16[2 * buffer->start_sample_offset];
    return result;
}

internal void SwitchAudioThreadProc(void *param) {
    Switch_Audio_State *audio_state = cast(Switch_Audio_State *) param;

    AudioDriver *driver = &audio_state->driver;
    while (__atomic_load_n(&audio_state->running, __ATOMIC_SEQ_CST)) {
        AudioDriverWaveBuf *buffer = SwitchGetFreeAudioBuffer(audio_state);
        if (buffer) {
            u32 samples_queued     = __atomic_load_n(&audio_state->samples_queued, __ATOMIC_SEQ_CST);
            u32 samples_to_dequeue = Min(samples_queued, audio_state->buffer_sample_count);
            u32 read_cursor        = audio_state->read_cursor;

            s16 *buffer_base = SwitchGetAudioBufferBase(buffer);

            if ((read_cursor + samples_to_dequeue) > audio_state->max_sample_count) {
                // Copy in two chunks
                //
                void *samples_src  = cast(void *) &audio_state->sample_queue[read_cursor];
                void *samples_dst  = cast(void *) buffer_base;
                u32   sample_count = (audio_state->max_sample_count - read_cursor);

                CopySize(samples_dst, samples_src, sample_count * sizeof(s16));

                samples_src  = cast(void *) audio_state->sample_queue;
                samples_dst  = cast(void *) &buffer_base[sample_count];
                sample_count = samples_to_dequeue - sample_count;

                CopySize(samples_dst, samples_src, sample_count * sizeof(s16));
            }
            else {
                // Copy in single chunk
                //
                void *samples_src = cast(void *) &audio_state->sample_queue[read_cursor];
                void *samples_dst = cast(void *) buffer_base;
                CopySize(samples_dst, samples_src, samples_to_dequeue * sizeof(s16));
            }

            if (samples_to_dequeue < audio_state->buffer_sample_count) {
                // Set the rest to silence if there aren't enough samples queued
                //
                s16 *silence_samples = &buffer_base[samples_to_dequeue];
                ZeroSize(silence_samples, (audio_state->buffer_sample_count - samples_to_dequeue) * sizeof(s16));
            }

            armDCacheFlush(buffer_base, audio_state->buffer_sample_count * sizeof(s16));

            __atomic_sub_fetch(&audio_state->samples_queued, samples_to_dequeue, __ATOMIC_SEQ_CST);

            audrvVoiceAddWaveBuf(driver, 0, buffer);

            audio_state->read_cursor += samples_to_dequeue;
            audio_state->read_cursor %= audio_state->max_sample_count;
        }

        audrvUpdate(driver);

        // Wait until the buffer actually starts playing to prevent re-queuing it next iteration
        //
        if (buffer) {
            while (buffer->state != AudioDriverWaveBufState_Playing) {
                audrvUpdate(driver);
                audrenWaitFrame();
            }
        }
        else {
            // If we didn't get a buffer to queue then find one that is still playing and wait for it to finish
            // So we can guarantee a free buffer next iteration
            //
            for (u32 it = 0; it < ArrayCount(audio_state->buffers); ++it) {
                if (audio_state->buffers[it].state == AudioDriverWaveBufState_Playing) {
                    buffer = &audio_state->buffers[it];
                    break;
                }
            }

            if (!buffer) { continue; }

            while (buffer->state == AudioDriverWaveBufState_Playing) {
                audrvUpdate(driver);
                audrenWaitFrame();
            }
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
        printf("[Error][Switch] :: Failed to initialise audio (0x%x)\n", rc);
        return result;
    }

    rc = audrvCreate(&audio_state->driver, &config, 2);
    if (R_FAILED(rc)) {
        printf("[Error][Switch] :: Failed to create audio driver (0x%x)\n", rc);
        return result;
    }

    audio_state->max_sample_count    = (sample_rate / 4);
    audio_state->samples_queued      = 0;
    audio_state->sample_queue        = cast(s16 *) malloc(audio_state->max_sample_count * sizeof(s16));
    ZeroSize(audio_state->sample_queue, audio_state->max_sample_count * sizeof(s16));

    audio_state->read_cursor         = 0;
    audio_state->write_cursor        = 0;

    audio_state->buffer_sample_count = channel_count * 1024;

    umm mem_pool_size = ArrayCount(audio_state->buffers) * (audio_state->buffer_sample_count * sizeof(s16));
    umm aligned_size  = (mem_pool_size + 0xFFF) & ~0xFFF; // Audio buffer memory has to be aligned to 4096 bytes

    void *mem_pool    = memalign(0x1000, aligned_size);

    ZeroSize(mem_pool, aligned_size);
    armDCacheFlush(mem_pool, aligned_size);

    audio_state->mem_pool_id = audrvMemPoolAdd(&audio_state->driver, mem_pool, aligned_size);
    audrvMemPoolAttach(&audio_state->driver, audio_state->mem_pool_id);

    u8 channel_ids[2] = { 0, 1 };
    audio_state->sink = audrvDeviceSinkAdd(&audio_state->driver, AUDREN_DEFAULT_DEVICE_NAME, 2, channel_ids);

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
        buffer->data_raw            = mem_pool;
        buffer->size                = mem_pool_size;
        buffer->start_sample_offset = (it * (audio_state->buffer_sample_count / channel_count));
        buffer->end_sample_offset   = (buffer->start_sample_offset + (audio_state->buffer_sample_count / channel_count));
    }

    audrvVoiceStart(&audio_state->driver, 0);
    audrvUpdate(&audio_state->driver);

    audio_state->running = true;

    rc = threadCreate(&audio_state->thread, SwitchAudioThreadProc, cast(void *) audio_state, 0, Megabytes(2), 0x3B, -2);
    if (R_FAILED(rc)) {
        printf("[Error][Switch] :: Failed to create audio thread (0x%x)\n", rc);
        return result;
    }

    rc = threadStart(&audio_state->thread);
    if (R_FAILED(rc)) {
        printf("[Error][Switch] :: Failed to start audio thread (0x%x)\n", rc);
        return result;
    }

    audio_state->enabled = true;

    result = true;
    return result;
}

internal void SwitchShutdownAudio(Switch_Audio_State *audio_state) {
    __atomic_store_n(&audio_state->running, false, __ATOMIC_SEQ_CST);
    threadWaitForExit(&audio_state->thread);

    threadClose(&audio_state->thread);

    if (audio_state->enabled) {
        audrvUpdate(&audio_state->driver);
        audrenWaitFrame();

        audrvVoiceStop(&audio_state->driver, 0);

        audrenStopAudioRenderer();

        audrvSinkRemove(&audio_state->driver, audio_state->sink);

        audrvMemPoolDetach(&audio_state->driver, audio_state->mem_pool_id);
        audrvMemPoolRemove(&audio_state->driver, audio_state->mem_pool_id);

        void *mem_pool = cast(void *) audio_state->buffers[0].data_raw;
        free(mem_pool);

        audrvClose(&audio_state->driver);

        audrenExit();
    }
}

internal void SwitchOutputSounds(Switch_Audio_State *audio_state, Game_Context *context, Sound_Buffer *sound_buffer) {
    u32 samples_to_queue = audio_state->max_sample_count - __atomic_load_n(&audio_state->samples_queued, __ATOMIC_SEQ_CST);
    if (samples_to_queue == 0) { return; } // Nothing to do

    sound_buffer->sample_count = samples_to_queue;
    LudumOutputSoundSamples(context, sound_buffer);

    u32 write_cursor = audio_state->write_cursor;
    if ((write_cursor + samples_to_queue) > audio_state->max_sample_count) {
        // Copy the first chunk to the end of the ring buffer
        //
        s16 *samples_dst = &audio_state->sample_queue[write_cursor];
        s16 *samples_src = sound_buffer->samples;
        u32 sample_count = (audio_state->max_sample_count - write_cursor);

        CopySize(samples_dst, samples_src, sample_count * sizeof(s16));

        armDCacheFlush(samples_dst, sample_count * sizeof(s16));

        // Copy the second wrapped portion of the ring buffer
        //
        //
        samples_dst  = audio_state->sample_queue;
        samples_src  = &sound_buffer->samples[sample_count];
        sample_count = samples_to_queue - sample_count;

        CopySize(samples_dst, samples_src, sample_count * sizeof(s16));

        armDCacheFlush(samples_dst, sample_count * sizeof(s16));
    }
    else {
        // Single chunk copy
        //
        s16 *samples = &audio_state->sample_queue[write_cursor];
        CopySize(samples, sound_buffer->samples, samples_to_queue * sizeof(s16));

        armDCacheFlush(samples, samples_to_queue * sizeof(s16));
    }

    __atomic_add_fetch(&audio_state->samples_queued, samples_to_queue, __ATOMIC_SEQ_CST);

    audio_state->write_cursor += samples_to_queue;
    audio_state->write_cursor %= audio_state->max_sample_count;
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

    Switch_Audio_State audio_state = {};
    if (!SwitchInitialiseAudio(&audio_state)) {
        printf("[Error] :: Failed to initialise audio\n");
        return 1;
    }

    Sound_Buffer sound_buffer = {};
    sound_buffer.sample_count = 12000;
    sound_buffer.samples      = cast(s16 *) malloc(sound_buffer.sample_count * sizeof(s16));
    ZeroSize(sound_buffer.samples, sound_buffer.sample_count * sizeof(s16));

    b32 running = true;

    // We have to render a single frame up front as there is a bug? or "optitimsation"? with egl where it will not
    // swap buffers (and thus wait for vsync) if no OpenGL calls are issued.
    //
    rect2 render_region;
    v2u window_size = SwitchGetWindowSize();

    v2 render_size;
    render_size.x = 4 * (window_size.y / 3);
    render_size.y = window_size.y;

    render_region.min = (0.5 * (V2(window_size) - render_size));
    render_region.max = render_region.min + render_size;

    Draw_Command_Buffer *command_buffer = renderer->BeginFrame(renderer, render_region);
    LudumUpdateRender(context, current_input, command_buffer);

    u64 start = SwitchGetCurrentTicks();

    f64 fixed_dt = 1.0f / 60.0f;
    f64 accum    = 0;

    while (running) {
        {
            u64 end = SwitchGetCurrentTicks();
            f64 dt  = SwitchGetElapsedTime(start, end);

            start = end;

            accum += dt;

            current_input->time = prev_input->time;
            current_input->time += dt;
        }

        SwitchHandleInput(&pad, current_input, prev_input);
        if (current_input->requested_quit) { running = false; }

        window_size = SwitchGetWindowSize();

        render_size.x = 4 * (window_size.y / 3);
        render_size.y = window_size.y;

        render_region.min = (0.5 * (V2(window_size) - render_size));
        render_region.max = render_region.min + render_size;

        while (accum >= fixed_dt) {
            command_buffer = renderer->BeginFrame(renderer, render_region);

            current_input->delta_time = fixed_dt;

            LudumUpdateRender(context, current_input, command_buffer);
            if (current_input->requested_quit) { running = false; }

            accum -= fixed_dt;
            if (accum < 0) { accum = 0; }
        }

        SwitchOutputSounds(&audio_state, context, &sound_buffer);

        renderer->EndFrame(renderer, command_buffer);

        Swap(current_input, prev_input);
    }

    renderer->Shutdown(renderer);

    SwitchShutdownAudio(&audio_state);

    if (global_nxlink_socket >= 0) {
        close(global_nxlink_socket);
        socketExit();
        global_nxlink_socket = -1;
    }

    return 0;
}
