#if !defined(LUDUM_UTILITY_H_)
#define LUDUM_UTILITY_H_

inline void CopySize(void *to, void *from, umm size) {
    u8 *to_ptr   = cast(u8 *) to;
    u8 *from_ptr = cast(u8 *) from;

    for (umm it = 0; it < size; ++it) {
        to_ptr[it] = from_ptr[it];
    }
}

inline void ZeroSize(void *ptr, umm size) {
    u8 *base = cast(u8 *) ptr;
    for (umm it = 0; it < size; ++it) {
        base[it] = 0;
    }
}

inline umm StringLength(u8 *str) {
    umm result = 0;
    while (str[result] != 0) {
        result += 1;
    }

    return result;
}

inline umm StringLength(const char *str) {
    umm result = 0;
    while (str[result] != 0) {
        result += 1;
    }

    return result;
}

inline string WrapZ(u8 *str) {
    string result;
    result.count = StringLength(str);
    result.data  = str;

    return result;
}

inline string WrapZ(const char *str) {
    string result;
    result.data  = cast(u8 *) str;
    result.count = StringLength(result.data);

    return result;
}

inline string WrapZ(char *str, umm count) {
    string result;
    result.count = count;
    result.data  = cast(u8 *) str;

    return result;
}

inline b32 EndsWith(string str, string ending) {
    b32 result = true;

    umm count = Min(str.count, ending.count);
    string end;
    end.count = count;
    end.data  = str.data + (str.count - end.count);

    for (umm it = 0; it < end.count; ++it) {
        if (end.data[it] != ending.data[it]) {
            result = false;
            break;
        }
    }

    return result;
}

inline b32 IsNumber(u8 c) {
    b32 result = (c >= '0') && (c <= '9');
    return result;
}

// @Todo(James): This is a *very* simple djb2 hash algorithm, can be made much better and we should probably
// use the aesdec intrinsics where available
//
inline u64 HashString(string value) {
    u64 result = 5381;
    for (umm it = 0; it < value.count; ++it) {
        result = ((result << 5) + result) + value.data[it];
    }

    return result;
}

#endif  // LUDUM_UTILITY_H_
