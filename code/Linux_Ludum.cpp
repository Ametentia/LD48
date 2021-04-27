#include <linux/limits.h>
#include <sys/mman.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <dlfcn.h>

#include <unistd.h>

#include <stdio.h>
#include <time.h>

#include <glob.h>

#include "Ludum.h"
#include "Ludum.cpp"

#include "SDL2_Ludum.cpp"

Platform_Context Platform;

struct Linux_File_List {
    Memory_Allocator alloc;
};

internal PLATFORM_ALLOCATE(LinuxAllocate) {
    Memory_Block *result = 0;

    umm full_size = size + sizeof(Memory_Block);
    result = cast(Memory_Block *) mmap(0, full_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

    result->base  = cast(u8 *) (result + 1);
    result->size  = size;
    result->used  = 0;
    result->flags = flags;
    result->prev  = 0;

    return result;
}

internal PLATFORM_DEALLOCATE(LinuxDeallocate) {
    // :MemoryFlags
    //
    munmap(block, block->size + sizeof(Memory_Block));
}

internal File_Info *LinuxAllocateFileInfo(File_List *list, struct stat *info, string basename) {
    File_Info *result;

    Linux_File_List *linux_list = cast(Linux_File_List *) list->platform;
    result = AllocStruct(&linux_list->alloc, File_Info);

    result->basename  = basename;
    result->timestamp = info->st_mtim.tv_sec + (1000000000 * info->st_mtim.tv_nsec);
    result->size      = info->st_size;

    result->next = list->first;
    list->first = result;

    list->file_count += 1;

    return result;
}

internal PLATFORM_FILE_ERROR(LinuxFileError) {
    printf("File error: %s\n", message);

    handle->errors = true;
}

internal PLATFORM_GET_FILE_LIST(LinuxGetFileList) {
    File_List result = {};

    Linux_File_List *linux_list = InlineAlloc(Linux_File_List, alloc);
    result.platform = cast(void *) linux_list;

    const char *wildcard = 0;
    switch (type) {
        case FileType_Amt: {
            wildcard = "data/*.amt";
        }
        break;
        case FileType_Config: {} break;
    }


    // @Todo: Just using glob for now... Can move away from it later
    //
    glob_t files;
    if (glob(wildcard, 0, 0, &files) != 0) {
        printf("[Error][Linux] :: Failed to list files\n");
        return result;
    }

    for (u32 it = 0; it < files.gl_pathc; ++it) {
        char *path = files.gl_pathv[it];

        struct stat file_stat;
        if (stat(path, &file_stat) != 0) {
            continue;
        }

        string filename = CopyString(&linux_list->alloc, WrapZ(path, StringLength(path) + 1));
        string basename;
        umm last_slash;
        for (umm c = 0; c < filename.count; ++c) {
            if (filename.data[c] == '.') {
                basename.count = c;
            }
            else if (filename.data[c] == '/') {
                basename.data = &filename.data[c + 1];
                last_slash = c;
            }
        }

        if (basename.count != 0) { basename.count -= (last_slash + 1); }

        File_Info *info = LinuxAllocateFileInfo(&result, &file_stat, basename);
        info->platform = cast(void *) filename.data;
    }

    globfree(&files);

    return result;
}

internal PLATFORM_CLEAR_FILE_LIST(LinuxClearFileList) {
    Linux_File_List *linux_list = cast(Linux_File_List *) list->platform;
    if (linux_list) {
        Clear(&linux_list->alloc);
    }

    list->platform = 0;
}

internal PLATFORM_OPEN_FILE(LinuxOpenFile) {
    File_Handle result = {};

    u32 open_flags = 0;
    if (access & FileAccess_Read) {
        open_flags |= O_RDONLY;
    }

    if (access & FileAccess_Write) {
        if (open_flags & O_RDONLY) {
            open_flags = O_RDWR;
        }
        else {
            open_flags |= O_WRONLY;
        }

        open_flags |= O_CREAT;
    }

    const char *filename = cast(const char *) info->platform;
    int fd = open(filename, open_flags);
    if (fd < 0) {
        LinuxFileError(&result, "Failed to open file");
    }
    else {
        *cast(int *) &result.platform = fd;
    }


    return result;
}

internal PLATFORM_CLOSE_FILE(LinuxCloseFile) {
    int fd = *cast(int *) &handle->platform;
    close(fd);
}

internal PLATFORM_READ_FILE(LinuxReadFile) {
    if (!IsValid(handle)) { return; }

    int fd = *cast(int *) &handle->platform;

    lseek(fd, offset, SEEK_SET);

    smm success = read(fd, data, size);
    if (success < 0) {
        LinuxFileError(handle, "Failed to read from file handle");
    }
    else if (success != size) {
        LinuxFileError(handle, "Incomplete read from file handle");
    }
}

internal PLATFORM_WRITE_FILE(LinuxWriteFile) {
    if (!IsValid(handle)) { return; }

    int fd = *cast(int *) &handle->platform;

    lseek(fd, offset, SEEK_SET);

    smm success = write(fd, data, size);
    if (success < 0) {
        LinuxFileError(handle, "Failed to write to file handle");
    }
    else if (success != size) {
        LinuxFileError(handle, "Incomplete write to file handle");
    }
}

internal b32 LinuxInitialise() {
    b32 result = false;

    printf("Hello, Linux!\n");

    umm path_count = 0;
    char exe_path[PATH_MAX];
    readlink("/proc/self/exe", exe_path, PATH_MAX);

    for (umm it = 0; exe_path[it] != 0; ++it) {
        if (exe_path[it] == '/') {
            path_count = it + 1;
        }
    }

    exe_path[path_count] = 0;

    chdir(exe_path);

    Platform.Allocate   = LinuxAllocate;
    Platform.Deallocate = LinuxDeallocate;

    Platform.FileError     = LinuxFileError;

    Platform.GetFileList   = LinuxGetFileList;
    Platform.ClearFileList = LinuxClearFileList;

    Platform.OpenFile      = LinuxOpenFile;
    Platform.CloseFile     = LinuxCloseFile;

    Platform.ReadFile      = LinuxReadFile;
    Platform.WriteFile     = LinuxWriteFile;

    result = SDL2Initialise(1024, 768, 0);
    return result;
}

int main(int argc, char **argv) {
    if (!LinuxInitialise()) {
        printf("[Error][Linux] :: Failed to initialise\n");
        return 1;
    }

    // Setup renderer parameters
    //
    Renderer_Parameters params = {};
    params.command_buffer_size          = Megabytes(1);
    params.max_immediate_vertex_count   = (1 << 17);
    params.max_immediate_index_count    = (1 << 18);
    params.max_texture_handle_count     = 128;
    params.texture_transfer_buffer_size = Megabytes(128);
    params.platform_data                = SDL2GetPlatformData();

    // Load renderer .so
    //
    void *renderer_so = dlopen("./Ludum_OpenGL.so", RTLD_NOW);
    if (!renderer_so) {
        printf("[Error][Linux] :: Failed to open 'Ludum_OpenGL.so'\n");
        return 1;
    }

    // Get the initalisation function pointer
    //
    Renderer_Initialise *RendererInitalise;
    RendererInitalise = cast(Renderer_Initialise *) dlsym(renderer_so, "LinuxOpenGLInitialise");
    if (!RendererInitalise) {
        printf("[Error][Linux] :: Failed to get initalisation function from 'Ludum_OpenGL.so'\n");
        return 1;
    }

    // Initialise renderer
    //
    Renderer_Context *renderer = RendererInitalise(&params);
    if (!IsValid(renderer)) {
        printf("[Error][Renderer] :: Failed to intialise renderer\n");
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
    Memory_Block *sound_buffer_memory = LinuxAllocate(sound_buffer_size, 0);

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

        // :RequestedQuit
        //
        if (current_input->requested_quit) { running = false; }

        rect2 render_region;
        v2u window_size = SDL2GetWindowSize();

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

        SDL2OutputSounds(context, sound_buffer);

        renderer->EndFrame(renderer, command_buffer);

        Swap(current_input, prev_input);
    }

    return 0;
}
