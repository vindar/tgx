#pragma once

#include "movie_scene.h"


struct AsteroidBody
    {
    uint8_t mesh_id;
    fVec3 base;
    fVec3 velocity;
    fVec3 axis;
    float scale;
    float spin_speed;
    float phase;
    };


struct AsteroidDraw
    {
    uint8_t mesh_id;
    fVec3 p;
    fVec3 axis;
    float scale;
    float angle;
    };


struct WingShipState
    {
    bool visible;
    fVec3 p;
    fVec3 v;
    float scale;
    float roll;
    };


static const AsteroidBody ASTEROIDS[] =
    {
    { 0, { -4.5f,  2.0f,   2.0f }, {  0.035f, -0.016f, -0.052f }, { 0.2f, 1.0f, 0.3f }, 1.05f,  20.0f,  0.0f },
    { 1, {  5.1f, -2.7f,  -5.5f }, { -0.030f,  0.021f, -0.046f }, { 1.0f, 0.2f, 0.3f }, 1.18f, -17.0f,  0.7f },
    { 0, { -4.3f, -3.1f, -12.0f }, {  0.026f,  0.014f, -0.038f }, { 0.5f, 0.7f, 1.0f }, 0.82f,  31.0f,  1.8f },
    { 1, {  4.6f,  2.9f, -18.0f }, { -0.024f, -0.018f, -0.050f }, { 0.8f, 0.4f, 0.1f }, 0.95f,  14.0f,  2.5f },
    { 0, {  4.8f,  0.9f, -24.0f }, { -0.036f,  0.020f, -0.044f }, { 0.3f, 1.0f, 0.2f }, 0.76f, -27.0f,  3.2f },
    { 1, { -5.5f, -2.2f, -30.0f }, {  0.022f, -0.021f, -0.040f }, { 0.4f, 0.5f, 1.0f }, 1.35f,  12.0f,  4.1f },
    { 0, {  3.4f, -2.9f, -35.0f }, {  0.018f,  0.023f, -0.047f }, { 1.0f, 0.1f, 0.5f }, 0.82f, -34.0f,  5.5f },
    { 1, { -3.5f,  2.8f, -39.0f }, { -0.026f, -0.019f, -0.042f }, { 0.3f, 0.9f, 0.8f }, 1.05f,  19.0f,  6.2f },
    { 0, {  5.7f, -4.1f, -43.0f }, { -0.032f,  0.018f, -0.034f }, { 0.7f, 0.3f, 1.0f }, 0.78f, -23.0f,  7.0f },
    { 1, { -6.7f,  3.9f, -46.5f }, {  0.028f, -0.024f, -0.036f }, { 1.0f, 0.6f, 0.2f }, 1.10f,  28.0f,  7.9f },

    // A deliberate central rock used for the orbit shot.  The ship path bends
    // around it with a wide radius.
    { 1, {  0.6f, -0.8f, -58.0f }, {  0.004f, -0.003f, -0.010f }, { 0.2f, 1.0f, 0.1f }, 2.15f,  9.0f,  1.5f },

    { 0, { -8.6f, -3.2f, -55.0f }, {  0.026f,  0.012f, -0.044f }, { 0.1f, 1.0f, 0.7f }, 0.70f,  16.0f,  9.3f },
    { 1, {  6.1f,  2.9f, -59.0f }, { -0.030f, -0.014f, -0.040f }, { 0.8f, 0.1f, 0.6f }, 1.22f, -15.0f, 10.2f },
    { 0, { -4.8f,  3.3f, -64.0f }, {  0.024f, -0.018f, -0.043f }, { 0.6f, 0.8f, 0.3f }, 0.88f,  25.0f, 11.3f },
    { 1, {  5.0f, -3.7f, -69.5f }, { -0.020f,  0.020f, -0.041f }, { 0.3f, 0.5f, 1.0f }, 0.96f, -21.0f, 12.0f },
    { 0, {  6.0f,  2.0f, -75.0f }, { -0.034f,  0.012f, -0.046f }, { 1.0f, 0.3f, 0.2f }, 0.72f,  18.0f, 12.8f },
    { 1, { -6.8f, -3.9f, -82.0f }, {  0.029f,  0.018f, -0.037f }, { 0.2f, 0.9f, 0.6f }, 1.15f, -13.0f, 13.6f },
    { 0, {  5.0f,  4.4f, -90.0f }, { -0.016f, -0.024f, -0.043f }, { 0.7f, 0.2f, 1.0f }, 0.86f,  22.0f, 14.7f },
    { 1, { -4.0f,  1.7f, -99.0f }, {  0.023f,  0.010f, -0.040f }, { 0.4f, 1.0f, 0.2f }, 1.05f,  17.0f, 15.4f },
    { 0, {  6.2f, -3.0f,-110.0f }, { -0.027f,  0.018f, -0.036f }, { 0.9f, 0.1f, 0.5f }, 0.70f, -24.0f, 16.8f },
    };

static const int NB_ASTEROIDS = sizeof(ASTEROIDS) / sizeof(ASTEROIDS[0]);
static const float HERO_RADIUS = 1.65f;
static const float WING_RADIUS = 0.72f;


fVec3 asteroidPosition(const AsteroidBody& a, float t)
    {
    return a.base + a.velocity * t;
    }


float asteroidRadius(const AsteroidBody& a)
    {
    return 1.05f * a.scale;
    }


bool isInFrontOfCamera(const CameraState& cam, const fVec3& p, float radius)
    {
    const CameraBasis b = makeCameraBasis(cam.eye, cam.target);
    const fVec3 d = p - cam.eye;
    const float z = dotv(d, b.forward);
    if (z < -radius || z > 430.0f) return false;
    const float x = fabsf(dotv(d, b.right));
    const float y = fabsf(dotv(d, b.up));
    const float spread = z * 1.15f + radius * 3.0f;
    return (x < spread && y < spread);
    }


bool asteroidDrawState(int index, float t, const CameraState& cam, AsteroidDraw& out)
    {
    const AsteroidBody& a = ASTEROIDS[index];
    out.mesh_id = a.mesh_id;
    out.p = asteroidPosition(a, t);
    out.axis = a.axis;
    out.scale = a.scale;
    out.angle = a.phase * 37.0f + t * a.spin_speed;
    return isInFrontOfCamera(cam, out.p, asteroidRadius(a));
    }


float minDistanceShipAsteroids()
    {
    float min_d = 10000.0f;
    for (int s = 0; s <= 240; s++)
        {
        const float t = MOVIE_SECONDS * (float)s / 240.0f;
        const ShipState ship = computeShipState(t);
        for (int i = 0; i < NB_ASTEROIDS; i++)
            {
            const float d = lenv(asteroidPosition(ASTEROIDS[i], t) - ship.p) - HERO_RADIUS - asteroidRadius(ASTEROIDS[i]);
            if (d < min_d) min_d = d;
            }
        }
    return min_d;
    }


WingShipState computeWingShip(float t, const ShipState& hero, int slot)
    {
    WingShipState w;
    w.visible = (t > 88.0f && t < 113.0f);
    if (!w.visible)
        {
        w.p = { 0.0f, 0.0f, 0.0f };
        w.v = hero.v;
        w.scale = 0.0f;
        w.roll = 0.0f;
        return w;
        }

    // The formation uses a delayed/smoothed reference path, not the hero's
    // instantaneous evasive motion.  This keeps the TIEs parallel and steady
    // while the X-wing can still move slightly inside the frame.
    ShipState anchor;
    anchor.p = sampleShipPath(clampf(t - 0.9f, 0.0f, MOVIE_SECONDS));
    anchor.v = normv(sampleShipVelocity(clampf(t - 2.0f, 0.0f, MOVIE_SECONDS)) +
                     sampleShipVelocity(clampf(t + 2.0f, 0.0f, MOVIE_SECONDS)));
    anchor.scale = hero.scale;
    anchor.roll = 0.0f;
    anchor.pitch = 0.0f;
    const CameraBasis b = makeShipBasis(anchor.v);
    const float join = smoother01((t - 88.0f) / 12.0f);
    const float leave = smooth01((t - 108.0f) / 5.0f);

    const float side = (slot == 0) ? -1.0f : ((slot == 1) ? 1.0f : 0.0f);
    const float height = (slot == 2) ? 1.35f : 0.58f;
    const float back = (slot == 2) ? 5.0f : 3.4f;
    const fVec3 far_offset = b.right * (side * 9.5f) - b.forward * (11.0f + slot * 3.0f) + b.up * (3.0f + 0.55f * slot);
    const fVec3 form_offset = b.right * (side * 3.05f) - b.forward * back + b.up * height;
    const fVec3 leave_offset = b.right * (side * 5.2f) - b.forward * (back + 18.0f * leave) + b.up * (height + 2.8f * leave);

    w.p = anchor.p + mixv(mixv(far_offset, form_offset, join), leave_offset, leave);
    w.v = anchor.v;
    w.scale = 0.48f + 0.08f * join;
    w.roll = -side * 5.0f;
    if (slot == 2)
        {
        if (t > 102.0f && t < 103.4f)
            {
            w.roll = 360.0f * smoother01((t - 102.0f) / 1.4f);
            }
        else if (t >= 103.4f)
            {
            w.roll = 0.0f;
            }
        }
    return w;
    }
