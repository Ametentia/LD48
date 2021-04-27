#if !defined(LUDUM_MATHS_H_)
#define LUDUM_MATHS_H_

struct Random {
    u64 state;
};

inline Random RandomSeed(u64 seed) {
    Random result = { seed };
    return result;
}

// Simple XorShift64
//
inline u64 NextRandom(Random *random) {
    u64 result = random->state;
    result ^= (result << 13);
    result ^= (result >> 7);
    result ^= (result << 17);

    random->state = result;
    return result;
}

inline f32 RandomUnilateral(Random *random) {
    f32 result = NextRandom(random) / cast(f32) U64_MAX;
    return result;
}

inline u32 RandomBetween(Random *random, u32 min, u32 max) {
    u32 result = min + cast(u32) (RandomUnilateral(random) * (max - min));
    return result;
}

inline f32 RandomBetween(Random *random, f32 min, f32 max) {
    f32 result = min + (RandomUnilateral(random) * (max - min));
    return result;
}

inline u32 RandomChoice(Random *random, u32 choices) {
    u32 result = NextRandom(random) % choices;
    return result;
}

inline u32 ABGRPack(v4 colour) {
    u32 result =
        ((cast(u8) (255.0f * colour.a)) << 24) |
        ((cast(u8) (255.0f * colour.b)) << 16) |
        ((cast(u8) (255.0f * colour.g)) <<  8) |
        ((cast(u8) (255.0f * colour.r)) <<  0);

    return result;
}

inline v2s V2S(s32 x, s32 y) {
    v2s result = { x, y };
    return result;
}

inline b32 IsEqual(v2s a, v2s b) {
    b32 result = (a.x == b.x) && (a.y == b.y);
    return result;
}

// v2u functions
//
inline v2u V2U(u32 x, u32 y) {
    v2u result = { x, y };
    return result;
}

inline v2u operator+(v2u a, v2u b) {
    v2u result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);

    return result;
}

inline v2u &operator+=(v2u &a, v2u b) {
    a = a + b;
    return a;
}

inline v2u operator-(v2u a, v2u b) {
    v2u result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);

    return result;
}

inline v2u &operator-=(v2u &a, v2u b) {
    a = a - b;
    return a;
}

inline v2u operator*(v2u a, v2u b) {
    v2u result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);

    return result;
}

inline v2u operator*(v2u a, u32 b) {
    v2u result;
    result.x = (b * a.x);
    result.y = (b * a.y);

    return result;
}

inline v2u operator*(u32 a, v2u b) {
    v2u result;
    result.x = (a * b.x);
    result.y = (a * b.y);

    return result;
}

inline v2u &operator*=(v2u &a, u32 b) {
    a = a * b;
    return a;
}

// v2 functions
//
inline v2 V2(f32 x, f32 y) {
    v2 result = { x, y };
    return result;
}

inline v2 V2(v2s xy) {
    v2 result = { cast(f32) xy.x, cast(f32) xy.y };
    return result;
}

inline v2 V2(v2u xy) {
    v2 result = { cast(f32) xy.x, cast(f32) xy.y };
    return result;
}

inline v2 V2(v3 xyz) {
    v2 result = { xyz.x, xyz.y };
    return result;
}

inline v2 operator+(v2 a, v2 b) {
    v2 result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);

    return result;
}

inline v2 &operator+=(v2 &a, v2 b) {
    a = a + b;
    return a;
}

inline v2 operator-(v2 a, v2 b) {
    v2 result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);

    return result;
}

inline v2 operator-(v2 a) {
    v2 result;
    result.x = -a.x;
    result.y = -a.y;

    return result;
}

inline v2 &operator-=(v2 &a, v2 b) {
    a = a - b;
    return a;
}

inline v2 operator*(v2 a, v2 b) {
    v2 result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);

    return result;
}

inline v2 operator*(v2 a, f32 b) {
    v2 result;
    result.x = (b * a.x);
    result.y = (b * a.y);

    return result;
}

inline v2 operator*(f32 a, v2 b) {
    v2 result;
    result.x = (a * b.x);
    result.y = (a * b.y);

    return result;
}

inline v2 &operator*=(v2 &a, v2 b) {
    a = a * b;
    return a;
}

inline v2 &operator*=(v2 &a, f32 b) {
    a = a * b;
    return a;
}

inline f32 Dot(v2 a, v2 b) {
    f32 result = (a.x * b.x) + (a.y * b.y);
    return result;
}

inline f32 LengthSq(v2 a) {
    f32 result = Dot(a, a);
    return result;
}

inline f32 Length(v2 a) {
    f32 result = Sqrt(Dot(a, a));
    return result;
}

inline v2 Normalise(v2 a) {
    v2 result = V2(0, 0);

    f32 len = Length(a);
    if (len != 0) {
        result = a * (1.0f / len);
    }

    return result;
}

inline v2 Perp(v2 a) {
    v2 result;
    result.x = -a.y;
    result.y =  a.x;

    return result;
}

inline v2 Arm2(f32 cos, f32 sin) {
    v2 result;
    result.x = cos;
    result.y = sin;

    return result;
}

inline v2 Arm2(f32 angle) {
    v2 result;
    result.x = Cos(angle);
    result.y = Sin(angle);

    return result;
}

inline v2 Rotate(v2 a, v2 rot) {
    v2 result;
    result.x = (a.x * rot.x) - (a.y * rot.y);
    result.y = (a.x * rot.y) + (a.y * rot.x);

    return result;
}

inline v2 Rotate(v2 a, f32 angle) {
    v2 result = Rotate(a, Arm2(angle));
    return result;
}

// v3 functions
//
inline v3 V3(f32 x, f32 y, f32 z) {
    v3 result = { x, y, z };
    return result;
}

inline v3 V3(v2 xy, f32 z = 0.0f) {
    v3 result = { xy.x, xy.y, z };
    return result;
}

inline v3 V3(v4 xyzw) {
    v3 result = { xyzw.x, xyzw.y, xyzw.z };
    return result;
}

inline v3 operator+(v3 a, v3 b) {
    v3 result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);
    result.z = (a.z + b.z);

    return result;
}

inline v3 &operator+=(v3 &a, v3 b) {
    a = a + b;
    return a;
}

inline v3 operator-(v3 a, v3 b) {
    v3 result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);
    result.z = (a.z - b.z);

    return result;
}

inline v3 operator-(v3 a) {
    v3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

inline v3 &operator-=(v3 &a, v3 b) {
    a = a - b;
    return a;
}

inline v3 operator*(v3 a, v3 b) {
    v3 result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);
    result.z = (a.z * b.z);

    return result;
}

inline v3 operator*(v3 a, f32 b) {
    v3 result;
    result.x = (b * a.x);
    result.y = (b * a.y);
    result.z = (b * a.z);

    return result;
}

inline v3 operator*(f32 a, v3 b) {
    v3 result;
    result.x = (a * b.x);
    result.y = (a * b.y);
    result.z = (a * b.z);

    return result;
}

inline v3 &operator*=(v3 &a, v3 b) {
    a = a * b;
    return a;
}

inline v3 &operator*=(v3 &a, f32 b) {
    a = a * b;
    return a;
}

inline f32 Dot(v3 a, v3 b) {
    f32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    return result;
}

inline f32 LengthSq(v3 a) {
    f32 result = Dot(a, a);
    return result;
}

inline f32 Length(v3 a) {
    f32 result = Sqrt(Dot(a, a));
    return result;
}

inline v3 Normalise(v3 a) {
    v3 result = V3(0, 0, 0);

    f32 len = Length(a);
    if (len != 0) {
        result = a * (1.0f / len);
    }

    return result;
}

inline v3 Cross(v3 a, v3 b) {
    v3 result;
    result.x = (a.y * b.z) - (a.z * b.y);
    result.y = (a.z * b.x) - (a.x * b.z);
    result.z = (a.x * b.y) - (a.y * b.x);

    return result;
}

// v4 functions
//
inline v4 V4(f32 x, f32 y, f32 z, f32 w) {
    v4 result = { x, y, z, w };
    return result;
}

inline v4 V4(v3 xyz, f32 w = 0.0f) {
    v4 result = { xyz.x, xyz.y, xyz.z, w };
    return result;
}

inline v4 operator+(v4 a, v4 b) {
    v4 result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);
    result.z = (a.z + b.z);
    result.w = (a.w + b.w);

    return result;
}

inline v4 &operator+=(v4 &a, v4 b) {
    a = a + b;
    return a;
}

inline v4 operator-(v4 a, v4 b) {
    v4 result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);
    result.z = (a.z - b.z);
    result.w = (a.w - b.w);

    return result;
}

inline v4 operator-(v4 a) {
    v4 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;

    return result;
}

inline v4 &operator-=(v4 &a, v4 b) {
    a = a - b;
    return a;
}

inline v4 operator*(v4 a, v4 b) {
    v4 result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);
    result.z = (a.z * b.z);
    result.w = (a.w * b.w);

    return result;
}

inline v4 operator*(v4 a, f32 b) {
    v4 result;
    result.x = (b * a.x);
    result.y = (b * a.y);
    result.z = (b * a.z);
    result.w = (b * a.w);

    return result;
}

inline v4 operator*(f32 a, v4 b) {
    v4 result;
    result.x = (a * b.x);
    result.y = (a * b.y);
    result.z = (a * b.z);
    result.w = (a * b.w);

    return result;
}

inline v4 &operator*=(v4 &a, v4 b) {
    a = a * b;
    return a;
}

inline v4 &operator*=(v4 &a, f32 b) {
    a = a * b;
    return a;
}

inline f32 Dot(v4 a, v4 b) {
    f32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    return result;
}

inline f32 LengthSq(v4 a) {
    f32 result = Dot(a, a);
    return result;
}

inline f32 Length(v4 a) {
    f32 result = Sqrt(Dot(a, a));
    return result;
}

inline v4 Normalise(v4 a) {
    v4 result = V4(0, 0, 0, 0);

    f32 len = Length(a);
    if (len != 0.0f) {
        result = a * (1.0f / len);
    }

    return result;
}

// mat4 functions
//
inline mat4 Identity() {
    mat4 result = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    return result;
}

inline v4 Transform(mat4 a, v4 b) {
    v4 result;
    result.x = (b.x * a.e[0][0]) + (b.y * a.e[0][1]) + (b.z * a.e[0][2]) + (b.w * a.e[0][3]);
    result.y = (b.x * a.e[1][0]) + (b.y * a.e[1][1]) + (b.z * a.e[1][2]) + (b.w * a.e[1][3]);
    result.z = (b.x * a.e[2][0]) + (b.y * a.e[2][1]) + (b.z * a.e[2][2]) + (b.w * a.e[2][3]);
    result.w = (b.x * a.e[3][0]) + (b.y * a.e[3][1]) + (b.z * a.e[3][2]) + (b.w * a.e[3][3]);

    return result;
}

inline void Translate(mat4 *a, v3 b) {
    a->e[0][3] += b.x;
    a->e[1][3] += b.y;
    a->e[2][3] += b.z;
}

inline mat4 operator*(mat4 a, mat4 b) {
    mat4 result;
    for (u32 r = 0; r < 4; ++r) {
        for (u32 c = 0; c < 4; ++c) {
            result.e[r][c] =
                (a.e[r][0] * b.e[0][c]) + (a.e[r][1] * b.e[1][c]) +
                (a.e[r][2] * b.e[2][c]) + (a.e[r][3] * b.e[3][c]);
        }
    }

    return result;
}

inline v4 operator*(mat4 a, v4 b) {
    v4 result = Transform(a, b);
    return result;
}

inline v3 operator*(mat4 a, v3 b) {
    v4 point = Transform(a, V4(b, 1.0f));

    v3 result = V3(point);
    return result;
}

inline mat4 XRotation(f32 angle) {
    f32 s = Sin(angle);
    f32 c = Cos(angle);

    mat4 result = {
        1, 0,  0, 0,
        0, c, -s, 0,
        0, s,  c, 0,
        0, 0,  0, 0
    };

    return result;
}

inline mat4 YRotation(f32 angle) {
    f32 s = Sin(angle);
    f32 c = Cos(angle);

    mat4 result = {
         c, 0,  s, 0,
         0, 1,  0, 0,
        -s, 0,  c, 0,
         0, 0,  0, 0
    };

    return result;
}

inline mat4 ZRotation(f32 angle) {
    f32 s = Sin(angle);
    f32 c = Cos(angle);

    mat4 result = {
        c, -s, 0, 0,
        s,  c, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 0
    };

    return result;
}

inline v3 GetRow(mat4 a, u32 row) {
    v3 result = V3(a.r[row]);
    return result;
}

inline v3 GetColumn(mat4 a, u32 col) {
    v3 result;
    result.x = a.e[0][col];
    result.y = a.e[1][col];
    result.z = a.e[2][col];

    return result;
}

inline mat4 Rows3x3(v3 x, v3 y, v3 z) {
    mat4 result = {
        x.x, x.y, x.z, 0,
        y.x, y.y, y.z, 0,
        z.x, z.y, z.z, 0,
        0,   0,   0,   1
    };

    return result;
}

inline mat4 Columns3x3(v3 x, v3 y, v3 z) {
    mat4 result = {
        x.x, y.x, z.x, 0,
        x.y, y.y, z.y, 0,
        x.z, y.z, z.z, 0,
        0,   0,   0,   1
    };

    return result;
}

inline mat4_inv OrthographicProjection(f32 aspect, f32 near, f32 far) {
    f32 a = 1.0;
    f32 b = -aspect;

    f32 c = 2.0f / (near - far);
    f32 d = (near + far) / (near - far);

    mat4_inv result = {
        // Forward
        //
        {
            a, 0, 0, 0,
            0, b, 0, 0,
            0, 0, c, d,
            0, 0, 0, 1
        },

        // Inverse
        //
        {
            (1.0f / a),  0,         0,           0,
             0,         (1.0f / b), 0,           0,
             0,          0,        (1.0f / c), -(d / c),
             0,          0,         0,           1
        }
    };

    return result;
}

inline mat4_inv PerspectiveProjection(f32 fov, f32 aspect, f32 near, f32 far) {
    f32 focal_len = (1.0f / Tan(0.5f * fov));

    f32 a = (focal_len / aspect);
    f32 b = (focal_len);

    f32 c = -(near + far) / (far - near);
    f32 d = -(2.0f * near * far) / (far - near);

    mat4_inv result = {
        // Forward
        //
        {
            a, 0,  0, 0,
            0, b,  0, 0,
            0, 0,  c, d,
            0, 0, -1, 0
        },

        // Inverse
        //
        {
            (1.0f / a),  0,          0,          0,
            0,          (1.0f / b),  0,          0,
            0,           0,          0,         -1,
            0,           0,         (1.0f / d), (c /d)
        }
    };

    return result;
}

inline mat4_inv CameraTransform(v3 x, v3 y, v3 z, v3 p) {
    mat4_inv result;

    result.forward = Rows3x3(x, y, z);

    v3 txp = -(result.forward * p);
    Translate(&result.forward, txp);

    v3 ix = x * (1.0f / LengthSq(x));
    v3 iy = y * (1.0f / LengthSq(y));
    v3 iz = z * (1.0f / LengthSq(z));
    v3 ip = V3((txp.x * ix.x) + (txp.y * iy.x) + (txp.z * iz.x),
               (txp.x * ix.y) + (txp.y * iy.y) + (txp.z * iz.y),
               (txp.x * ix.z) + (txp.y * iy.z) + (txp.z * iz.z));

    result.inverse = Columns3x3(x, y, z);
    Translate(&result.inverse, -ip);

    return result;
}

inline rect2 Rect2(rect3 r) {
    rect2 result;
    result.min = V2(r.min);
    result.max = V2(r.max);

    return result;
}

inline b32 Overlaps(rect2 a, rect2 b) {
    b32 result =
        (a.max.x > b.min.x) &&
        (a.min.x < b.max.x) &&
        (a.max.y > b.min.y) &&
        (a.min.y < b.max.y);

    return result;
}

inline b32 Contains(rect2 a, v2 p) {
    b32 result =
        (a.min.x < p.x) && (a.max.x > p.x)  &&
        (a.min.y < p.y) && (a.max.y > p.y);

    return result;
}

#endif  // LUDUM_MATHS_H_
