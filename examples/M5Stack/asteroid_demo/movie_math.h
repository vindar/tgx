#pragma once

#include <math.h>
#include <tgx.h>

using tgx::fVec3;

static const float MOVIE_PI = 3.14159265358979323846f;
static const float MOVIE_RAD_TO_DEG = 57.295779513082320876f;


inline float clampf(float x, float a, float b)
    {
    if (x < a) return a;
    if (x > b) return b;
    return x;
    }


inline float smooth01(float x)
    {
    x = clampf(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
    }


inline float smoother01(float x)
    {
    x = clampf(x, 0.0f, 1.0f);
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
    }


inline float fadeInOut(float t, float in0, float in1, float out0, float out1)
    {
    return smooth01((t - in0) / (in1 - in0)) * (1.0f - smooth01((t - out0) / (out1 - out0)));
    }


inline float smoothRange(float t, float a, float b)
    {
    return smooth01((t - a) / (b - a));
    }


inline float mixf(float a, float b, float u)
    {
    return a + (b - a) * u;
    }


inline fVec3 mixv(const fVec3& a, const fVec3& b, float u)
    {
    return a + (b - a) * u;
    }


inline float hermitef(float p0, float p1, float m0, float m1, float u)
    {
    const float u2 = u * u;
    const float u3 = u2 * u;
    return (2.0f * u3 - 3.0f * u2 + 1.0f) * p0 +
           (u3 - 2.0f * u2 + u) * m0 +
           (-2.0f * u3 + 3.0f * u2) * p1 +
           (u3 - u2) * m1;
    }


inline fVec3 hermitev(const fVec3& p0, const fVec3& p1, const fVec3& m0, const fVec3& m1, float u)
    {
    const float u2 = u * u;
    const float u3 = u2 * u;
    return p0 * (2.0f * u3 - 3.0f * u2 + 1.0f) +
           m0 * (u3 - 2.0f * u2 + u) +
           p1 * (-2.0f * u3 + 3.0f * u2) +
           m1 * (u3 - u2);
    }


inline float dotv(const fVec3& a, const fVec3& b)
    {
    return a.x * b.x + a.y * b.y + a.z * b.z;
    }


inline fVec3 crossv(const fVec3& a, const fVec3& b)
    {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
    }


inline float lenv(const fVec3& v)
    {
    return sqrtf(dotv(v, v));
    }


inline fVec3 normv(const fVec3& v)
    {
    const float d = lenv(v);
    if (d < 0.000001f) return { 0.0f, 0.0f, -1.0f };
    return v * (1.0f / d);
    }


inline fVec3 catmullRom(const fVec3& p0, const fVec3& p1, const fVec3& p2, const fVec3& p3, float u)
    {
    const float u2 = u * u;
    const float u3 = u2 * u;
    return (p1 * 2.0f +
            (p2 - p0) * u +
            (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * u2 +
            (-p0 + p1 * 3.0f - p2 * 3.0f + p3) * u3) * 0.5f;
    }


inline fVec3 catmullRomVelocity(const fVec3& p0, const fVec3& p1, const fVec3& p2, const fVec3& p3, float u)
    {
    const float u2 = u * u;
    return ((p2 - p0) +
            (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * (2.0f * u) +
            (-p0 + p1 * 3.0f - p2 * 3.0f + p3) * (3.0f * u2)) * 0.5f;
    }


struct CameraBasis
    {
    fVec3 forward;
    fVec3 right;
    fVec3 up;
    };


inline CameraBasis makeCameraBasis(const fVec3& eye, const fVec3& target)
    {
    CameraBasis b;
    b.forward = normv(target - eye);
    b.right = normv(crossv(b.forward, { 0.0f, 1.0f, 0.0f }));
    if (lenv(b.right) < 0.0001f) b.right = { 1.0f, 0.0f, 0.0f };
    b.up = normv(crossv(b.right, b.forward));
    return b;
    }


inline CameraBasis makeShipBasis(const fVec3& velocity)
    {
    CameraBasis b;
    b.forward = normv(velocity);
    b.right = normv(crossv(b.forward, { 0.0f, 1.0f, 0.0f }));
    if (lenv(b.right) < 0.0001f) b.right = { 1.0f, 0.0f, 0.0f };
    b.up = normv(crossv(b.right, b.forward));
    return b;
    }


inline float yawForDirection(const fVec3& v)
    {
    // The source mesh points toward +Z, while the movie path moves mostly in -Z.
    return 180.0f + atan2f(v.x, v.z) * MOVIE_RAD_TO_DEG;
    }


inline float pitchForDirection(const fVec3& v)
    {
    const float h = sqrtf(v.x * v.x + v.z * v.z);
    return -atan2f(v.y, h) * MOVIE_RAD_TO_DEG;
    }


inline uint32_t hash32(uint32_t x)
    {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
    }
