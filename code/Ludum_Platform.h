#if !defined(LUDUM_PLATFORM_H_)
#define LUDUM_PLATFORM_H_

enum Mouse_Button {
    MouseButton_Left = 0,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_X1,
    MouseButton_X2,

    MouseButton_Count
};

struct Game_Button {
    b32 pressed;
    u32 transistions;
};

struct Game_Controller {
    b32 connected;
    b32 analog;

    // @Note: This is kinda game specific will likely change heavily
    //
    union {
        struct {
            Game_Button up;
            Game_Button down;
            Game_Button left;
            Game_Button right;

            Game_Button action;
            Game_Button interact;

            Game_Button menu;
        };

        Game_Button buttons[7];
    };
};

struct Game_Input {
    f64 delta_time;
    b32 requested_quit;

    // 1 keyboard, 4 gamepads
    // Index 0 is *always* keyboard if platform uses it
    //
    Game_Controller controllers[5];

    v2 mouse_clip;
    v2 mouse_delta;

    Game_Button mouse_buttons[MouseButton_Count];

    // Mainly for debug, offset by 1 to sync with actual F keys. f[0] is never used
    //
    Game_Button f[13];
};

// Get a controller of a given index
//
inline Game_Controller *GameGetController(Game_Input *input, u32 index) {
    Game_Controller *result = &input->controllers[index];
    return result;
}

// Returns true if the button is pressed
//
inline b32 IsPressed(Game_Button button) {
    b32 result = (button.pressed);
    return result;
}

// Returns true if the button is pressed this frame
//
inline b32 JustPressed(Game_Button button) {
    b32 result = (button.pressed) && (button.transistions != 0);
    return result;
}

// Returns true if the button was released this frame
//
inline b32 WasPressed(Game_Button button) {
    b32 result = (!button.pressed) && (button.transistions != 0);
    return result;
}

union Memory_Block {
    struct {
        u8 *base;
        umm size;
        umm used;

        u64 flags;

        Memory_Block *prev;
    };

    u8 pad[64]; // Pad to cache line size
};

#define PLATFORM_ALLOCATE(name) Memory_Block *name(umm size, u64 flags)
#define PLATFORM_DEALLOCATE(name) void name(Memory_Block *block)

typedef PLATFORM_ALLOCATE(Platform_Allocate);
typedef PLATFORM_DEALLOCATE(Platform_Deallocate);

enum File_Type {
    FileType_Amt = 0,
    FileType_Config
};

enum File_Access_Flags {
    FileAccess_Read  = 0x1,
    FileAccess_Write = 0x2,
};

struct File_Info {
    string basename;

    u64 timestamp;
    umm size;

    void *platform;

    File_Info *next;
};

struct File_List {
    u32 file_count;
    File_Info *first;

    void *platform;
};

struct File_Handle {
    b32 errors;
    void *platform;
};

inline b32 IsValid(File_Handle *handle) {
    b32 result = !(handle->errors);
    return result;
}

#define PLATFORM_FILE_ERROR(name) void name(File_Handle *handle, const char *message)

typedef PLATFORM_FILE_ERROR(Platform_File_Error);

#define PLATFORM_GET_FILE_LIST(name) File_List name(File_Type type)
#define PLATFORM_CLEAR_FILE_LIST(name) void name(File_List *list)

typedef PLATFORM_GET_FILE_LIST(Platform_Get_File_List);
typedef PLATFORM_CLEAR_FILE_LIST(Platform_Clear_File_List);

#define PLATFORM_OPEN_FILE(name) File_Handle name(File_Info *info, u32 access)
#define PLATFORM_CLOSE_FILE(name) void name(File_Handle *handle)

typedef PLATFORM_OPEN_FILE(Platform_Open_File);
typedef PLATFORM_CLOSE_FILE(Platform_Close_File);

#define PLATFORM_READ_FILE(name) void name(File_Handle *handle, void *data, umm offset, umm size)
#define PLATFORM_WRITE_FILE(name) void name(File_Handle *handle, void *data, umm offset, umm size)

typedef PLATFORM_READ_FILE(Platform_Read_File);
typedef PLATFORM_WRITE_FILE(Platform_Write_File);

struct Platform_Context {
    Platform_Allocate   *Allocate;
    Platform_Deallocate *Deallocate;

    Platform_File_Error      *FileError;

    Platform_Get_File_List   *GetFileList;
    Platform_Clear_File_List *ClearFileList;

    Platform_Open_File       *OpenFile;
    Platform_Close_File      *CloseFile;

    Platform_Read_File       *ReadFile;
    Platform_Write_File      *WriteFile;
};

extern Platform_Context Platform;

struct Sound_Buffer {
    u32 sample_count;
    s16 *samples;
};

struct Game_State;
struct Texture_Transfer_Buffer;

struct Game_Context {
    Game_State *state;

    Texture_Transfer_Buffer *texture_buffer;
};

#endif  // LUDUM_PLATFORM_H_
