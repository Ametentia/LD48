#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include <time.h>

#include <stdlib.h>

// Dumb Microsoft stuff defining these as empty macros
//
#undef near
#undef far

#include "Ludum.h"
#include "Ludum.cpp"

#include "SDL2_Ludum.cpp"

Platform_Context Platform;

struct Win_File_List {
    Memory_Allocator alloc;
};

internal PLATFORM_ALLOCATE(WinAllocate) {
    Memory_Block *result = 0;

    umm full_size = size + sizeof(Memory_Block);
    result = cast(Memory_Block *) VirtualAlloc(0, full_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    result->base  = cast(u8 *) (result + 1);
    result->size  = size;
    result->used  = 0;
    result->flags = flags;
    result->prev  = 0;

    return result;
}

internal PLATFORM_DEALLOCATE(WinDeallocate) {
    // :MemoryFlags
    // @Note: We have access to the flags used to allocate the flags here so if we need to use them to deallocate
    // then we can
    //
    VirtualFree(block, 0, MEM_RELEASE);
}

internal string WinUTF16ToUTF8(Win_File_List *list, wchar_t *name, umm name_count) {
    string result;

    result.data  = cast(u8 *) AllocSize(&list->alloc, 4 * name_count);
    result.count = WideCharToMultiByte(CP_UTF8, 0, name, name_count,
            cast(LPSTR) result.data, 4 * name_count, 0, 0);

    return result;
}

internal File_Info *WinAllocateFileInfo(File_List *list, WIN32_FIND_DATAW *info, string basename) {
    Win_File_List *win_list = cast(Win_File_List *) list->platform;

    File_Info *result = AllocStruct(&win_list->alloc, File_Info);
    result->basename  = basename;
    result->size      = cast(umm) (cast(u64) info->nFileSizeHigh << 32) | info->nFileSizeLow;
    result->timestamp = 0;

    result->next = list->first;
    list->first  = result;
    list->file_count += 1;

    return result;
}

internal PLATFORM_FILE_ERROR(WinFileError) {
    OutputDebugString(message);
    OutputDebugString("\n");

    handle->errors = true;
}

internal PLATFORM_GET_FILE_LIST(WinGetFileList) {
    File_List result = {};

    Win_File_List *win_list = InlineAlloc(Win_File_List, alloc);
    result.platform = cast(void *) win_list;

    const wchar_t *wildcard = 0;
    const wchar_t *dir = 0;
    switch (type) {
        case FileType_Amt: {
            wildcard = L"data\\*.amt";
            dir      = L"data\\";
        }
        break;
        case FileType_Config: {
            // @Todo(James): Add support for a configuration file type
            //
        }
        break;
    }

    umm dir_count = 0;
    while (dir[dir_count] != 0) { dir_count += 1; }

    WIN32_FIND_DATAW find_data = {};
    HANDLE find_handle = FindFirstFileW(wildcard, &find_data);
    while (find_handle != INVALID_HANDLE_VALUE) {
        wchar_t *filename  = find_data.cFileName;
        umm basename_count = 0;
        umm filename_count = 0;
        while (filename[filename_count] != 0) {
            if (filename[filename_count] == L'.') {
                basename_count = filename_count;
            }

            filename_count += 1;
        }

        if (!basename_count) { basename_count = filename_count; }

        string basename = WinUTF16ToUTF8(win_list, filename, basename_count);
        File_Info *info = WinAllocateFileInfo(&result, &find_data, basename);

        umm full_path_count  = dir_count + filename_count;
        wchar_t *full_path = AllocArray(&win_list->alloc, wchar_t, full_path_count + sizeof(wchar_t));

        CopySize(full_path, cast(void *) dir, dir_count * sizeof(wchar_t));
        CopySize(full_path + dir_count, filename, filename_count * sizeof(wchar_t));
        full_path[full_path_count] = 0;

        info->platform = cast(void *) full_path;

        if (!FindNextFileW(find_handle, &find_data)) {
            break;
        }
    }

    return result;
}

internal PLATFORM_CLEAR_FILE_LIST(WinClearFileList) {
    Win_File_List *win_list = cast(Win_File_List *) list->platform;
    if (win_list) {
        Clear(&win_list->alloc);
    }

    list->platform = 0;
}

internal PLATFORM_OPEN_FILE(WinOpenFile) {
    File_Handle result = {};

    DWORD access_flags = 0;
    DWORD create_flags = 0;
    if (access & FileAccess_Read) {
        access_flags |= GENERIC_READ;
        create_flags  = OPEN_EXISTING;
    }

    if (access & FileAccess_Write) {
        access_flags |= GENERIC_WRITE;
        create_flags  = OPEN_ALWAYS;
    }

    const wchar_t *filename = cast(const wchar_t *) info->platform;
    HANDLE win_handle = CreateFileW(filename, access_flags, FILE_SHARE_READ, 0, create_flags, 0, 0);
    result.errors = (win_handle == INVALID_HANDLE_VALUE);
    *cast(HANDLE *) &result.platform = win_handle;

    return result;
}

internal PLATFORM_CLOSE_FILE(WinCloseFile) {
    HANDLE win_handle = *cast(HANDLE *) &handle->platform;
    if (win_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(win_handle);
    }

    handle->errors   = true;
    handle->platform = 0;
}

internal PLATFORM_READ_FILE(WinReadFile) {
    HANDLE win_handle = *cast(HANDLE *) &handle->platform;
    if (win_handle == INVALID_HANDLE_VALUE) {
        WinFileError(handle, "Attempted to read from invalid file handle");
        return;
    }

    OVERLAPPED overlapped = {};
    overlapped.Offset     = cast(DWORD) (offset & 0xFFFFFFFF);
    overlapped.OffsetHigh = cast(DWORD) ((offset >> 32) & 0xFFFFFFFF);

    DWORD actual_size = cast(DWORD) size;

    DWORD size_read = 0;
    BOOL success = ReadFile(win_handle, data, actual_size, &size_read, &overlapped);
    if (!success) {
        WinFileError(handle, "Failed to read from file handle");
    }
    else if (size_read != actual_size) {
        WinFileError(handle, "Incomplete read from file handle");
    }
}

internal PLATFORM_WRITE_FILE(WinWriteFile) {
    HANDLE win_handle = *cast(HANDLE *) &handle->platform;
    if (win_handle == INVALID_HANDLE_VALUE) {
        WinFileError(handle, "Attempted to write to invalid file handle");
        return;
    }

    OVERLAPPED overlapped = {};
    overlapped.Offset     = cast(DWORD) (offset & 0xFFFFFFFF);
    overlapped.OffsetHigh = cast(DWORD) ((offset >> 32) & 0xFFFFFFFF);

    DWORD actual_size = cast(DWORD) size;

    DWORD size_written = 0;
    BOOL success = WriteFile(win_handle, data, actual_size, &size_written, &overlapped);
    if (!success) {
        WinFileError(handle, "Failed to write to file handle");
    }
    else if (size_written != actual_size) {
        WinFileError(handle, "Incomplete write to file handle");
    }
}

internal b32 WinInitialise() {
    b32 result = false;

    OutputDebugString("Hello, Windows!\n");

    HMODULE exe = GetModuleHandleA(0);

    DWORD path_count = 0;
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(exe, exe_path, MAX_PATH);

    for (DWORD it = 0; exe_path[it] != 0; ++it) {
        if (exe_path[it] == L'\\') {
            path_count = it;
        }
    }

    exe_path[path_count + 1] = 0;

    SetCurrentDirectoryW(exe_path);

    Platform.Allocate   = WinAllocate;
    Platform.Deallocate = WinDeallocate;

    Platform.FileError     = WinFileError;

    Platform.GetFileList   = WinGetFileList;
    Platform.ClearFileList = WinClearFileList;

    Platform.OpenFile      = WinOpenFile;
    Platform.CloseFile     = WinCloseFile;

    Platform.ReadFile      = WinReadFile;
    Platform.WriteFile     = WinWriteFile;

    // @Todo(James): We should have a configuration file to load these from so we can edit them without merge
    // conflicts and easy changes
    //
    result = SDL2Initialise(1280, 720, 1);
    return result;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (!WinInitialise()) {
        OutputDebugString("[Error][Windows] :: Failed to initialise\n");
        return 1;
    }

    // Setup renderer parameters
    //
    Renderer_Parameters params = {};
    params.command_buffer_size          = Megabytes(1);
    params.max_immediate_vertex_count   = (1 << 16);
    params.max_immediate_index_count    = (1 << 17);
    params.max_texture_handle_count     = 128;
    params.texture_transfer_buffer_size = Megabytes(128);
    params.platform_data                = SDL2GetPlatformData();

    // Load renderer .dll
    //
    HMODULE renderer_dll = LoadLibraryA("Ludum_OpenGL.dll");
    if (!renderer_dll) {
        OutputDebugString("[Error][Windows] :: Failed to load 'Ludum_OpenGL.dll'\n");
        return 1;
    }

    // Get the initalisation function pointer
    //
    Renderer_Initialise *RendererInitalise;
    RendererInitalise = cast(Renderer_Initialise *) GetProcAddress(renderer_dll, "WinOpenGLInitialise");
    if (!RendererInitalise) {
        OutputDebugString("[Error][Windows] :: Failed to get initalisation function from 'Ludum_OpenGL.dll'\n");
        return 1;
    }

    // Initialise renderer
    //
    Renderer_Context *renderer = RendererInitalise(&params);
    if (!IsValid(renderer)) {
        OutputDebugString("[Error][Renderer] :: Failed to intialise renderer\n");
        return 1;
    }

    Game_Input inputs[2]      = { 0 };
    Game_Input *current_input = &inputs[0];
    Game_Input *prev_input    = &inputs[1];

    // Keyboard always assumed to be connected on PC
    //
    GameGetController(current_input, 0)->connected = true;
    GameGetController(prev_input,    0)->connected = true;

    // Setup game context
    // This is used to transfer more 'permanent' things between the platform layer and the game code
    //
    Game_Context _context = {};
    Game_Context *context = &_context;

    context->state          = 0;
    context->texture_buffer = &renderer->texture_buffer;

    // Setup sound buffer
    //
    Sound_Buffer _sound_buffer;
    Sound_Buffer *sound_buffer = &_sound_buffer;

    // @Note: This is 250ms of latency at 48KHz. This should be safe, could be a configuration paramter
    // later on
    //
    umm sound_buffer_size = 12000 * sizeof(s16);
    Memory_Block *sound_buffer_memory = WinAllocate(sound_buffer_size, 0);

    sound_buffer->sample_count = 12000;
    sound_buffer->samples      = cast(s16 *) sound_buffer_memory->base;

    // Set the game as running
    //
    b32 running = true;

    // Begin timing information
    //
    u64 start = SDL2GetCurrentTicks();
    while (running) {
        {
            // Delta time calculation
            //
            u64 end = SDL2GetCurrentTicks();
            current_input->delta_time = SDL2GetElapsedTime(start, end);
            start = end;
        }

        SDL2HandleInput(current_input, prev_input);

        // :RequestedQuit requested_quit *will* be passed to the game code, this means we can do stuff like force save
        // the game or clean up state when quitting to prevent data loss etc.
        //
        if (current_input->requested_quit) { running = false; }

        v2u window_size = SDL2GetWindowSize();
        Draw_Command_Buffer *command_buffer = renderer->BeginFrame(renderer, window_size);

        // @Todo(James): Fix clock. This doesn't have any sort of time synchronisation
        //
        LudumUpdateRender(context, current_input, command_buffer);
        if (current_input->requested_quit) { running = false; }

        SDL2OutputSounds(context, sound_buffer);

        renderer->EndFrame(renderer, command_buffer);

        Swap(current_input, prev_input);
    }

    return 0;
}
