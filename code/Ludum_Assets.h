#if !defined(LUDUM_ASSETS_H_)
#define LUDUM_ASSETS_H_

// @Note: Asset pack file information
//
#define AMTF_CODE(a, b, c, d) (((u32) d) << 24 | ((u32) c) << 16 | ((u32) b) << 8 | ((u32) a) << 0)
#define AMTF_SIGNATURE AMTF_CODE('A', 'M', 'T', 'F')
#define AMTF_VERSION (1)

#pragma pack(push, 1)
struct Amt_Header {
    u32 signature;
    u32 version;
    u32 asset_count;
    u32 pad32[5];

    u64 string_table_size;
    u64 pad64[3];
};

struct Amt_Image {
    u32 width;
    u32 height;
};

struct Amt_Sound {
    u32 channel_count;
    u32 sample_count;
};

struct Amt_Asset {
    u32 type;
    union {
        Amt_Image image;
        Amt_Sound sound;

        u8 pad[44];
    };

    u64 data_offset;
    u64 data_size;
};
#pragma pack(pop)

enum Asset_Type {
    AssetType_None = 0,
    AssetType_Image,
    AssetType_Sound
};

enum Asset_State {
    AssetState_Unloaded = 0,
    AssetState_Loaded
};

struct Asset_File {
    File_Handle handle;

    u32 base_asset_index;
    Amt_Header header;
};

struct Asset {
    u32 state;

    u32 type;

    u32 file_index;
    u32 asset_index;

    Amt_Asset amt;

    union {
        Texture_Handle texture;
        s16 *sound_samples;
    };
};

struct Asset_Hash {
    u64 hash_value;
    u32 index;

    Asset_Hash *next;
};

struct Asset_Manager {
    Memory_Allocator *alloc;

    Texture_Transfer_Buffer *texture_transfer_buffer;

    u32 file_count;
    Asset_File *asset_files;

    u32 asset_count;
    Asset *assets;

    u32 next_texture_handle;

    Asset_Hash hash_slots[256];
};

struct Image_Handle {
    u32 value;
};

struct Sound_Handle {
    u32 value;
};

// @Todo(James): Animation is here because Ludum_Assets.h is included after Ludum_Renderer.h so defining
// this with an Image_Handle inside it at the bottom of Ludum_Renderer.h causes undefined type issues because
// C++ is crap
//
// I should just organise where everything needs to go
//
struct Animation {
    Image_Handle image;

    f32 time;
    f32 time_per_frame;

    u32 rows;
    u32 columns;
    u32 total_frames;

    u32 current_frame;
};

inline b32 IsValid(Image_Handle handle) {
    b32 result = (handle.value != 0);
    return result;
}

inline b32 IsValid(Sound_Handle handle) {
    b32 result = (handle.value != 0);
    return result;
}

// Manually tell the asset manager to load an asset attached to the given handle
//
internal void LoadImage(Asset_Manager *manager, Image_Handle handle);
internal void LoadSound(Asset_Manager *manager, Sound_Handle handle);

// Get information about an asset via a given handle
//
// image: [width, height]
// sound: [channel count, sample count]
//
// @Note: For now we only support stereo sounds so channel count *should* always be 2
//
internal Amt_Image *GetImageInfo(Asset_Manager *manager, Image_Handle handle);
internal Amt_Sound *GetSoundInfo(Asset_Manager *manager, Sound_Handle handle);

// Get a handle to either an image or sound using its string name. hash table lookup
//
internal Image_Handle GetImageByName(Asset_Manager *manager, const char *name);
internal Sound_Handle GetSoundByName(Asset_Manager *manager, const char *name);

// @Note: Sound output stuff
// @Todo(James): This will probably move somewhere else in the future
//
enum Playing_Sound_Flags {
    PlayingSound_Looped = 0x1,
};

struct Playing_Sound {
    Sound_Handle handle;

    f32 volume;
    u32 samples_played;
    u32 flags;

    Playing_Sound *next;
};

// Add a sound to the list of playing sounds
//
// All the sounds will be mixed into the sound output buffer
// Handle can be retrieved from the asset system with its name
//
internal Playing_Sound *PlaySound(Game_State *state, Sound_Handle handle, f32 volume, u32 flags);

#endif  // LUDUM_ASSETS_H_
