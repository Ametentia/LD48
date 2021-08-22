#if !defined(LUDUM_INTRINSICS_H_)
#define LUDUM_INTRINSICS_H_

#include <math.h>

#if ARCH_AMD64

inline f32 Sqrt(f32 x) {
    f32 result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set1_ps(x)));
    return result;
}

inline f32 ReciprocalSqrt(f32 x) {
    f32 result = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set1_ps(x)));
    return result;
}

#elif ARCH_AARCH64

inline f32 Sqrt(f32 x) {
    f32 result = vgetq_lane_f32(vsqrtq_f32(vdupq_n_f32(x)), 0);
    return result;
}

inline f32 ReciprocalSqrt(f32 x) {
    f32 result = vrsqrtes_f32(x); // I'm not sure why there are f32 versions of this but not for sqrt
    return result;
}

#endif

inline f32 Sin(f32 x) {
    f32 result = sinf(x);
    return result;
}

inline f32 Cos(f32 x) {
    f32 result = cosf(x);
    return result;
}

inline f32 Tan(f32 x) {
    f32 result = tanf(x);
    return result;
}

#endif  // LUDUM_INTRINSICS_H_
