#if !defined(LUDUM_TYPES_H_)
#define LUDUM_TYPES_H_

#include <stdint.h>
#include <float.h>

// Architecture detection
//
#define ARCH_AMD64   0
#define ARCH_AARCH64 0

#if defined(_M_AMD64) || defined(__amd64)
#undef  ARCH_AMD64
#define ARCH_AMD64 1
#elif defined(_M_ARM64) || defined(__aarch64__)
#undef  ARCH_AARCH64
#define ARCH_AARCH64 1
#else
#error "Unsupported architecture. Only amd64 and aarch64 are supported."
#endif

// Compiler detection
//
#define COMPILER_MSVC 0
#define COMPILER_GNU  0
#define COMPILER_LLVM 0

#if defined(_MSC_VER)
#undef  COMPILER_MSVC
#define COMPILER_MSVC 1
#elif defined(__clang__) && defined(__cplusplus)
#undef  COMPILER_LLVM
#define COMPILER_LLVM 1
#elif defined(__GNUG__)
#undef  COMPILER_GNU
#define COMPILER_GNU 1
#else
#error "Unsupported compiler. Only cl.exe, clang++ and g++ are supported."
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uintptr_t umm;
typedef intptr_t  smm;

typedef float  f32;
typedef double f64;

typedef u32 b32;

struct buffer {
    umm count;
    u8 *data;
};

typedef buffer string;

union v2s {
    struct {
        s32 x, y;
    };

    struct {
        s32 w, h;
    };

    s32 e[2];
};


union v2u {
    struct {
        u32 x, y;
    };

    struct {
        u32 w, h;
    };

    u32 e[2];
};

union v2 {
    struct {
        f32 x, y;
    };

    struct {
        f32 u, v;
    };

    struct {
        f32 w, h;
    };

    f32 e[2];
};

union v3 {
    struct {
        f32 x, y, z;
    };

    struct {
        f32 r, g, b;
    };

    struct {
        f32 w, h, d;
    };

    f32 e[3];
};

union v4 {
    struct {
        f32 x, y, z;
        f32 w;
    };

    struct {
        f32 r, g, b;
        f32 a;
    };

    f32 e[4];
};

struct Textured_Vertex {
    v3 position;
    v2 uv;
    u32 colour;
};

union mat4 {
    struct {
        f32 e[4][4];
    };

    struct {
        v4 r[4];
    };

    f32 m[16];
};

struct mat4_inv {
    mat4 forward;
    mat4 inverse;
};

struct rect2 {
    v2 min;
    v2 max;
};

struct rect3 {
    v3 min;
    v3 max;
};

#define local    static
#define global   static
#define internal static

#define cast(x) (x)
#define ArrayCount(x) ((sizeof(x) / sizeof((x)[0])))
#define OffsetOf(type, member) ((umm) &(((type *) 0)->member))
#define Swap(a, b) { auto __temp = a; a = b; b = __temp; }

#define Kilobytes(x) (1024LL * (x))
#define Megabytes(x) (1024LL * Kilobytes(x))
#define Gigabytes(x) (1024LL * Megabytes(x))
#define Terabytes(x) (1024LL * Gigabytes(x))

#define PI32 (3.14159265358979323846264338327950288419716939937510582f)
#define Radians(x) ((x) * (PI32 / 180.0f))
#define Degrees(x) ((x) * (180.0f / PI32))

#define U64_MAX (UINT64_MAX)
#define U32_MAX (UINT32_MAX)

#if ARCH_AMD64
    #if COMPILER_MSVC
        #include <intrin.h>
        #define __BREAKPOINT __debugbreak()
    #else
        #define __BREAKPOINT asm("int3")
        #include <x86intrin.h>
    #endif
#elif ARCH_AARCH64
    // @Todo(James): Figure out if there are any includes needed for NEON
    //
    #define __BREAKPOINT asm("BKPT")
#endif

#if LUDUM_DEBUG
#define Assert(exp) do { if (!(exp)) { __BREAKPOINT; } } while (0)
#else
#define Assert(exp)
#endif

#endif  // LUDUM_TYPES_H_
