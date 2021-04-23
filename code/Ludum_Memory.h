#if !defined(LUDUM_MEMORY_H_)
#define LUDUM_MEMORY_H_

enum Allocation_Flags {
    Allocation_NoClear = 0x1,
};

struct Memory_Allocator {
    Memory_Block *block;

    u64 flags;
    umm min_block_size;

    u32 temp_count;
};

struct Allocation_Parameters {
    umm alignment;
    u64 flags;
};

struct Temporary_Memory {
    Memory_Allocator *alloc;
    Memory_Block *block;
    umm block_used;
};

inline Allocation_Parameters AllocDefault() {
    Allocation_Parameters result;
    result.alignment = 4;
    result.flags     = 0;

    return result;
}

inline Allocation_Parameters AllocNoClear(umm alignment = 4) {
    Allocation_Parameters result;
    result.alignment = alignment;
    result.flags     = Allocation_NoClear;

    return result;
}

inline void SetMinimumBlockSize(Memory_Allocator *alloc, umm size) {
    alloc->min_block_size = size;
}

inline umm GetAlignmentOffset(Memory_Allocator *alloc, umm alignment) {
    umm result = 0;

    umm align_mask = alignment - 1;
    umm base_ptr   = cast(umm) (alloc->block->base + alloc->block->used);
    if (base_ptr & align_mask) {
        result = alignment - (base_ptr & align_mask);
    }

    return result;
}

inline umm GetAlignedSize(Memory_Allocator *alloc, umm size, umm alignment) {
    umm result = size;
    result += GetAlignmentOffset(alloc, size);

    return result;
}

// These macros are what you should be using to allocate memory
// The type is the actual struct name/typename
//
#define AllocStruct(alloc, type, ...) (type *) __AllocSize(alloc, sizeof(type), ##__VA_ARGS__)
#define AllocArray(alloc, type, count, ...) (type *) __AllocSize(alloc, (count) * sizeof(type), ##__VA_ARGS__)
#define AllocSize(alloc, count, ...) __AllocSize(alloc, (count), ##__VA_ARGS__)
inline void *__AllocSize(Memory_Allocator *alloc, umm size, Allocation_Parameters params = AllocDefault()) {
    void *result = 0;

    umm full_size = (alloc->block) ? GetAlignedSize(alloc, size, params.alignment) : 0;
    if (!alloc->block || ((alloc->block->used + full_size) > alloc->block->size)) {
        full_size = size;

        if (!alloc->min_block_size) {
            SetMinimumBlockSize(alloc, Megabytes(1));
        }

        umm block_size = Max(full_size, alloc->min_block_size);

        // :MemoryFlags
        // @Reserved: Flags can be used in the future
        //
        Memory_Block *block = Platform.Allocate(block_size, 0);
        block->prev  = alloc->block;
        alloc->block = block;
    }

    umm offset = GetAlignmentOffset(alloc, params.alignment);
    result = cast(void *) (alloc->block->base + (alloc->block->used + offset));
    alloc->block->used += full_size;

    if (!(params.flags & Allocation_NoClear)) {
        ZeroSize(result, size);
    }

    return result;
}

inline string CopyString(Memory_Allocator *alloc, string str) {
    string result;
    result.count = str.count;
    result.data  = cast(u8 *) AllocSize(alloc, result.count);
    CopySize(result.data, str.data, result.count);

    return result;
}

inline void DeallocateBlock(Memory_Allocator *alloc) {
    Memory_Block *block = alloc->block;
    if (block) {
        alloc->block = block->prev;
        Platform.Deallocate(block);
    }
}

inline void Clear(Memory_Allocator *alloc) {
    while (alloc->block) {
        b32 last = (alloc->block->prev == 0);

        DeallocateBlock(alloc);
        if (last) { break; }
    }
}

inline Temporary_Memory BeginTemp(Memory_Allocator *alloc) {
    Temporary_Memory result;

    result.alloc = alloc;
    result.block = alloc->block;

    if (alloc->block) {
        result.block_used = alloc->block->used;
    }

    alloc->temp_count += 1;
    return result;
}

inline void EndTemp(Temporary_Memory temp) {
    Memory_Allocator *alloc = temp.alloc;

    while (alloc->block != temp.block) {
        DeallocateBlock(alloc);
    }

    if (alloc->block) {
        alloc->block->used = temp.block_used;
    }

    alloc->temp_count -= 1;
}

#define InlineAlloc(type, alloc_name, ...) \
    (type *) __InlineAlloc(sizeof(type), OffsetOf(type, alloc_name), ##__VA_ARGS__)

inline void *__InlineAlloc(umm size, umm alloc_offset, Allocation_Parameters params = AllocDefault()) {
    void *result = 0;

    Memory_Allocator alloc = {};
    result = AllocSize(&alloc, size, params);
    *cast(Memory_Allocator *) ((cast(u8 *) result) + alloc_offset) = alloc;

    return result;
}

#endif  // LUDUM_MEMORY_H_
