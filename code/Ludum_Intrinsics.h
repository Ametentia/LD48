#if !defined(LUDUM_INTRINSICS_H_)
#define LUDUM_INTRINSICS_H_

#include <math.h>

#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Abs(a) (((a) < 0) ? -(a) : (a))
#define Lerp(a, b, alpha) (((alpha) * (a)) + ((1 - (alpha)) * (b)))

inline f32 Sqrt(f32 x) {
    f32 result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set1_ps(x)));
    return result;
}

inline f32 ReciprocalSqrt(f32 x) {
    f32 result = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set1_ps(x)));
    return result;
}

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
