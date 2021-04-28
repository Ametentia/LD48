#define SDL_MAIN_HANDLED 1
#include <SDL2/SDL.h>

global SDL_Window *global_window;
global SDL_GameController *global_controller_handles[4];
global SDL_AudioDeviceID global_audio_device;
global f64 global_performance_frequency;
global b32 global_fullscreen;

internal b32 SDL2Initialise(u32 width, u32 height, u32 display_index = 0) {
    b32 result = false;

    // @Note: SDL2 fails to initialise on Windows due to some group policy issue if sensors are included,
    // as we don't need them just remove it
    //
    u32 init_flags = SDL_INIT_EVERYTHING & ~SDL_INIT_SENSOR;
    if (SDL_Init(init_flags) != 0) {
        // @Todo(James): Logging...
        //
        return result;
    }

    // @Note: This isn't always needed if we end up supporting other graphics apis in the future however,
    // for basic fullscreen multi-sampling to work in OpenGL using SDL2 you have to set these parameters
    // *before* creating the window. Other OpenGL parameters are set when the actual OpenGL context is
    // created by the renderer
    //
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,    1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,    4);

    u32 window_flags = SDL_WINDOW_OPENGL;
    u32 x = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    u32 y = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);

    global_window = SDL_CreateWindow("Orpheus", x, y, width, height, window_flags);
    if (!global_window) {
        // @Todo(James): Logging...
        //
        return result;
    }

    global_performance_frequency = cast(f64) SDL_GetPerformanceFrequency();

    SDL_AudioSpec desired;
    SDL_AudioSpec acquired;

    desired.freq     = 48000;
    desired.format   = AUDIO_S16LSB;
    desired.channels = 2;
    desired.samples  = 512;
    desired.callback = 0;
    desired.userdata = 0;

    global_audio_device = SDL_OpenAudioDevice(0, 0, &desired, &acquired, 0);
    if (global_audio_device != 0) {
        SDL_PauseAudioDevice(global_audio_device, 0);
    }
    else {
        // @Todo: Logging...
        //
    }

    result = true;
    return result;
}

internal v2u SDL2GetWindowSize() {
    v2u result;

    s32 width, height;
    SDL_GetWindowSize(global_window, &width, &height);

    result.w = cast(u32) width;
    result.h = cast(u32) height;

    return result;
}

internal void SDL2ToggleFullscreen() {
    if (global_fullscreen) {
        SDL_SetWindowFullscreen(global_window, 0);
    }
    else {
        SDL_SetWindowFullscreen(global_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

    global_fullscreen = !global_fullscreen;
}

internal void SDL2HandleButtonPress(Game_Button *button, b32 pressed) {
    if (button->pressed != pressed) {
        button->pressed       = pressed;
        button->transistions += 1;
    }
}

internal f32 SDL2GetControllerAxisValue(SDL_GameController *handle, SDL_GameControllerAxis axis, f32 deadzone) {
    f32 result = 0.0f;

    s16 value = SDL_GameControllerGetAxis(handle, axis);
    if (value >= 0) {
        f32 norm = cast(f32) value / 32767.0f;
        if (norm >= deadzone) { result = norm; }
    }
    else {
        f32 norm = cast(f32) value / 32768.0f;
        if (norm <= -deadzone) { result = norm; }
    }

    return result;
}

internal void SDL2HandleKeyboardEvent(Game_Controller *current, SDL_KeyboardEvent *event) {
    SDL_Keycode key_code = event->keysym.sym;
    u16 modifiers        = event->keysym.mod;
    b32 pressed          = (event->state == SDL_PRESSED);

    // Can be changed to accommodate the buttons we actually need
    // @Todo(James): I would very much like to have re-bindable keys in the future
    //
    if (key_code == SDLK_w)      { SDL2HandleButtonPress(&current->up,       pressed); }
    else if (key_code == SDLK_s) { SDL2HandleButtonPress(&current->down,     pressed); }
    else if (key_code == SDLK_a) { SDL2HandleButtonPress(&current->left,     pressed); }
    else if (key_code == SDLK_d) { SDL2HandleButtonPress(&current->right,    pressed); }
    else if (key_code == SDLK_e) { SDL2HandleButtonPress(&current->interact, pressed); }
    else if (key_code == SDLK_q) { SDL2HandleButtonPress(&current->action,   pressed); }
    else if (key_code == SDLK_ESCAPE) { SDL2HandleButtonPress(&current->menu, pressed); }
    else if (key_code == SDLK_f) {
        if (pressed && (event->repeat == 0) && (modifiers & KMOD_LALT)) {
            SDL2ToggleFullscreen();
        }
    }
}

internal void SDL2HandleInput(Game_Input *current_input, Game_Input *prev_input) {
    Game_Controller *current_kb = GameGetController(current_input, 0);
    Game_Controller *prev_kb    = GameGetController(prev_input,    0);

    for (u32 it = 0; it < ArrayCount(Game_Controller::buttons); ++it) {
        current_kb->buttons[it].pressed      = prev_kb->buttons[it].pressed;
        current_kb->buttons[it].transistions = 0;
    }

    current_input->mouse_clip.z = prev_input->mouse_clip.z;

    //
    // Event handling
    //
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                current_input->requested_quit = true;
            }
            break;

            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                SDL2HandleKeyboardEvent(current_kb, &event.key);
            }
            break;

            case SDL_MOUSEWHEEL: {
                current_input->mouse_clip.z -= event.wheel.y;
            }
            break;

            case SDL_CONTROLLERDEVICEADDED: {
                s32 index = -1;
                for (u32 it = 0; it < ArrayCount(global_controller_handles); ++it) {
                    if (!global_controller_handles[it]) {
                        index = it;
                        break;
                    }
                }

                if (index != -1) {
                    SDL_GameController *handle = SDL_GameControllerOpen(event.cdevice.which);
                    if (handle) {
                        global_controller_handles[index] = handle;

                        Game_Controller *current = GameGetController(current_input, index + 1);
                        Game_Controller *prev    = GameGetController(prev_input,    index + 1);

                        current->connected = current->analog = true;
                        prev->connected    = prev->analog    = true;
                    }
                }
            }
            break;

            case SDL_CONTROLLERDEVICEREMOVED: {
                SDL_JoystickID id = cast(SDL_JoystickID) event.cdevice.which;
                SDL_GameController *handle = SDL_GameControllerFromInstanceID(id);

                for (u32 it = 0; it < ArrayCount(global_controller_handles); ++it) {
                    if (global_controller_handles[it] == handle) {
                        Game_Controller *current = GameGetController(current_input, it + 1);
                        Game_Controller *prev    = GameGetController(prev_input,    it + 1);

                        current->connected = current->analog = false;
                        prev->connected    = prev->analog    = false;

                        SDL_GameControllerClose(handle);
                        global_controller_handles[it] = 0;
                        break;
                    }
                }
            }
            break;
        }
    }

    // Just the F keys are handled here for debug via polling.
    // Could technically do all keyboard handling here but we will just use events for actual input
    //
    s32 key_count;
    const u8 *keys = SDL_GetKeyboardState(&key_count);

    for (u32 it = 0; it < ArrayCount(Game_Input::f) - 1; ++it) {
        current_input->f[it + 1].pressed      = prev_input->f[it + 1].pressed;
        current_input->f[it + 1].transistions = 0;

        b32 pressed = (keys[SDL_SCANCODE_F1 + it] != 0);
        SDL2HandleButtonPress(&current_input->f[it + 1], pressed);
    }

    //
    // Mouse input handling
    //
    s32 mouse_x, mouse_y;
    u32 mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

    v2u window_size = SDL2GetWindowSize();

    current_input->mouse_clip.x = -1.0f + ((2.0f * mouse_x) / cast(f32) window_size.w);
    current_input->mouse_clip.y =  1.0f - ((2.0f * mouse_y) / cast(f32) window_size.h);

    current_input->mouse_delta = current_input->mouse_clip - prev_input->mouse_clip;

    for (u32 it = 0; it < MouseButton_Count; ++it) {
        current_input->mouse_buttons[it].pressed      = prev_input->mouse_buttons[it].pressed;
        current_input->mouse_buttons[it].transistions = 0;

        b32 pressed = (mouse_buttons & (1 << it)) != 0;
        SDL2HandleButtonPress(&current_input->mouse_buttons[it], pressed);
    }

    //
    // Controller input handling
    // @Todo: The game code doesn't currently get the actual analog value of axes which would be useful
    //
    for (u32 it = 0; it < ArrayCount(global_controller_handles); ++it) {
        SDL_GameController *handle = global_controller_handles[it];
        if (!handle) { continue; }

        Game_Controller *current = GameGetController(current_input, it + 1);
        Game_Controller *prev    = GameGetController(prev_input,    it + 1);

        for (u32 button = 0; button < ArrayCount(Game_Controller::buttons); ++button) {
            current->buttons[button].pressed      = prev->buttons[button].pressed;
            current->buttons[button].transistions = 0;
        }

        b32 pressed;
        v2 left_stick;
        left_stick.x = SDL2GetControllerAxisValue(handle, SDL_CONTROLLER_AXIS_LEFTX, 0.05f);
        left_stick.y = SDL2GetControllerAxisValue(handle, SDL_CONTROLLER_AXIS_LEFTY, 0.05f);

        pressed = SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_UP) != 0;
        SDL2HandleButtonPress(&current->up, (left_stick.y < -0.5f) || pressed);

        pressed = SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_DOWN) != 0;
        SDL2HandleButtonPress(&current->down, (left_stick.y > 0.5f) || pressed);

        pressed = SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_LEFT) != 0;
        SDL2HandleButtonPress(&current->left, (left_stick.x < -0.5f) || pressed);

        pressed = SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) != 0;
        SDL2HandleButtonPress(&current->right, (left_stick.x > 0.5f) || pressed);

        pressed = SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_A) != 0;
        SDL2HandleButtonPress(&current->action, pressed);

        pressed = SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_X) != 0;
        SDL2HandleButtonPress(&current->interact, pressed);

        pressed = SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_START) != 0;
        SDL2HandleButtonPress(&current->menu, pressed);
    }
}

internal void SDL2OutputSounds(Game_Context *context, Sound_Buffer *sound_buffer) {
    const u32 max_sample_count = 12000;
    umm frame_sample_size = (max_sample_count * sizeof(s16)) - SDL_GetQueuedAudioSize(global_audio_device);

    sound_buffer->sample_count = frame_sample_size / sizeof(s16);
    LudumOutputSoundSamples(context, sound_buffer);

    if (global_audio_device != 0) {
        SDL_QueueAudio(global_audio_device, sound_buffer->samples, frame_sample_size);
    }
}

internal u64 SDL2GetCurrentTicks() {
    u64 result = SDL_GetPerformanceCounter();
    return result;
}

internal f64 SDL2GetElapsedTime(u64 start, u64 end) {
    f64 result = cast(f64) (end - start) / global_performance_frequency;
    return result;
}

internal void *SDL2GetPlatformData() {
    void *result = cast(void *) global_window;
    return result;
}
