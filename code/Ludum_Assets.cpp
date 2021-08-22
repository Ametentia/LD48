internal b32 IsValid(Asset_Hash *slot) {
    b32 result = (slot->index != 0) && (slot->hash_value != 0);
    return result;
}

internal void AddAssetToHash(Asset_Manager *manager, u32 index, string name) {
    u64 hash_value = HashString(name);
    u32 hash_index = hash_value % ArrayCount(manager->hash_slots);

    Asset_Hash *slot = &manager->hash_slots[hash_index];
    if (IsValid(slot)) {
        Asset_Hash *next = AllocStruct(manager->alloc, Asset_Hash);
        next->hash_value = hash_value;
        next->index      = index;

        next->next = slot->next;
        slot->next = next;
    }
    else {
        slot->hash_value = hash_value;
        slot->index      = index;

        slot->next  = 0;
    }
}

internal void InitialiseAssetManager(Game_State *state, Asset_Manager *manager,
        Texture_Transfer_Buffer *texture_transfer_buffer)
{
    manager->alloc = &state->perm;
    manager->texture_transfer_buffer = texture_transfer_buffer;

    Memory_Allocator *temp = state->temp;
    Temporary_Memory asset_temp = BeginTemp(temp);

    File_List file_list = Platform.GetFileList(FileType_Amt);
    manager->asset_files = AllocArray(manager->alloc, Asset_File, file_list.file_count);

    manager->asset_count = 1;
    manager->next_texture_handle = 1;

    for (File_Info *info = file_list.first; info; info = info->next) {
        Asset_File *file = &manager->asset_files[manager->file_count];
        manager->file_count += 1;

        file->handle = Platform.OpenFile(info, FileAccess_Read);
        if (!IsValid(&file->handle)) { continue; }

        Platform.ReadFile(&file->handle, &file->header, 0, sizeof(Amt_Header));

        if (file->header.signature != AMTF_SIGNATURE) {
            Platform.FileError(&file->handle, "Incorrect signature in asset file");
            continue;
        }

        if (file->header.version != AMTF_VERSION) {
            Platform.FileError(&file->handle, "Unsupported version in asset file");
            continue;
        }

        file->base_asset_index  = manager->asset_count;
        manager->asset_count   += file->header.asset_count;
    }

    Platform.ClearFileList(&file_list);

    u32 running_asset_index = 1;
    manager->assets = AllocArray(manager->alloc, Asset, manager->asset_count);
    for (u32 it = 0; it < manager->file_count; ++it) {
        Asset_File *file = &manager->asset_files[it];
        if (!IsValid(&file->handle)) { continue; }

        u8 *string_data = cast(u8 *) AllocSize(temp, file->header.string_table_size, AllocNoClear());

        Platform.ReadFile(&file->handle, string_data, sizeof(Amt_Header), file->header.string_table_size);
        if (!IsValid(&file->handle)) { continue; }

        umm amt_assets_size   = sizeof(Amt_Asset)  * file->header.asset_count;
        umm amt_assets_offset = sizeof(Amt_Header) + file->header.string_table_size;

        Amt_Asset *amt_assets = AllocArray(temp, Amt_Asset, file->header.asset_count, AllocNoClear());

        Platform.ReadFile(&file->handle, amt_assets, amt_assets_offset, amt_assets_size);
        if (!IsValid(&file->handle)) { continue; }

        for (u32 asset_index = 0; asset_index < file->header.asset_count; ++asset_index) {
            Amt_Asset *amt = &amt_assets[asset_index];

            string asset_name;
            asset_name.count = *cast(u64 *) string_data;
            asset_name.data  = (string_data + sizeof(u64));

            string_data = (asset_name.data + asset_name.count);

            Asset *asset = &manager->assets[running_asset_index];
            asset->state       = AssetState_Unloaded;
            asset->type        = amt->type;
            asset->file_index  = it;
            asset->asset_index = asset_index;
            asset->amt         = *amt;

            AddAssetToHash(manager, running_asset_index, asset_name);

            running_asset_index += 1;
        }
    }

    EndTemp(asset_temp);
}

internal Texture_Handle AcquireTextureHandle(Asset_Manager *manager, u32 width, u32 height) {
    Texture_Handle result;
    result.index  = manager->next_texture_handle; // @Todo: Re-use old handles!
    result.width  = cast(u16) width;
    result.height = cast(u16) height;

    manager->next_texture_handle += 1;

    return result;
}

internal Asset *GetAssetFromHandleValue(Asset_Manager *manager, u32 value) {
    // @Todo: Check if actually in bounds
    //
    Asset *result = &manager->assets[value];
    return result;
}

internal void LoadImage(Asset_Manager *manager, Image_Handle handle) {
    Asset *asset = GetAssetFromHandleValue(manager, handle.value);
    if (asset->state == AssetState_Unloaded) {
        Amt_Image *info = &asset->amt.image;

        Asset_File *file = &manager->asset_files[asset->file_index];
        if (!IsValid(&file->handle)) { return; }

        Texture_Transfer_Info *transfer_info =
            GetTextureTransferMemory(manager->texture_transfer_buffer, info->width, info->height);

        if (transfer_info) {
            u8 *transfer_base = cast(u8 *) (transfer_info + 1);
            Platform.ReadFile(&file->handle, transfer_base, asset->amt.data_offset, transfer_info->transfer_size);

            asset->texture = AcquireTextureHandle(manager, info->width, info->height);
            transfer_info->handle = asset->texture;
        }

        asset->state = AssetState_Loaded;
    }
}

internal void LoadSound(Asset_Manager *manager, Sound_Handle handle) {
    Asset *asset = GetAssetFromHandleValue(manager, handle.value);
    if (asset->state == AssetState_Unloaded) {
        // @Todo(James): This will need to change a bit if we want to do asset streaming and unloading
        // as we just allocate the memory for the sound and keep it there once its loaded and it is
        // never freed
        //

        Asset_File *file = &manager->asset_files[asset->file_index];
        umm total_sound_size = asset->amt.data_size;
        asset->sound_samples = cast(s16 *) AllocSize(manager->alloc, total_sound_size);

        Platform.ReadFile(&file->handle, asset->sound_samples, asset->amt.data_offset, total_sound_size);

        asset->state = AssetState_Loaded;
    }
}

internal Amt_Image *GetImageInfo(Asset_Manager *manager, Image_Handle handle) {
    Amt_Image *result;

    Asset *asset = GetAssetFromHandleValue(manager, handle.value);
    Assert(asset->type == AssetType_Image);

    result = &asset->amt.image;

    return result;
}

internal Amt_Sound *GetSoundInfo(Asset_Manager *manager, Sound_Handle handle) {
    Amt_Sound *result;

    Asset *asset = GetAssetFromHandleValue(manager, handle.value);
    Assert(asset->type == AssetType_Sound);

    result = &asset->amt.sound;

    return result;
}

internal Texture_Handle GetImageData(Asset_Manager *manager, Image_Handle handle) {
    Texture_Handle result = {};

    if (!IsValid(handle)) { return result; }

    Asset *asset = GetAssetFromHandleValue(manager, handle.value);
    Assert(asset->type == AssetType_Image);

    if (asset->state == AssetState_Unloaded) {
        LoadImage(manager, handle);
    }

    result = asset->texture;
    return result;
}

internal s16 *GetSoundData(Asset_Manager *manager, Sound_Handle handle) {
    s16 *result = 0;

    if (!IsValid(handle)) { return result; }

    Asset *asset = GetAssetFromHandleValue(manager, handle.value);
    Assert(asset->type == AssetType_Sound);

    if (asset->state == AssetState_Unloaded) {
        LoadSound(manager, handle);
    }

    result = asset->sound_samples;
    return result;
}

internal u32 GetAssetIndexByName(Asset_Manager *manager, string name) {
    u32 result = 0;

    u64 hash_value = HashString(name);
    u32 hash_index = hash_value % ArrayCount(manager->hash_slots);

    Asset_Hash *slot = &manager->hash_slots[hash_index];
    while (slot && slot->hash_value != hash_value) {
        slot = slot->next;
    }

    if (slot) {
        result = slot->index;
    }

    return result;
}

internal Image_Handle GetImageByName(Asset_Manager *manager, const char *name) {
    Image_Handle result;
    result.value = GetAssetIndexByName(manager, WrapZ(name));

    return result;
}

internal Sound_Handle GetSoundByName(Asset_Manager *manager, const char *name) {
    Sound_Handle result;
    result.value = GetAssetIndexByName(manager, WrapZ(name));

    return result;
}

// @Todo: Move elsewhere when we have a more solid audio mixer setup
//
internal Playing_Sound *PlaySound(Game_State *state, Sound_Handle handle, f32 volume = 1, u32 flags = 0) {
    Playing_Sound *result = 0;

    // Freelist for simplicity, probably want to swap to slot-based
    //
    if (state->free_playing_sounds) {
        result = state->free_playing_sounds;
        state->free_playing_sounds = result->next;
    }
    else {
        result = AllocStruct(&state->perm, Playing_Sound);
    }

    result->handle = handle;
    result->volume = volume;
    result->flags  = flags;
    result->samples_played = 0;

    result->next = state->playing_sounds;
    state->playing_sounds = result;

    return result;
}
