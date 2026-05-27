#pragma once

#include "movie_math.h"

static const float MOVIE_SECONDS = 120.0f;
static const uint32_t MOVIE_MS = 120000u;


struct ShipState
    {
    fVec3 p;
    fVec3 v;
    float scale;
    float roll;
    float pitch;
    };


struct CameraState
    {
    fVec3 eye;
    fVec3 target;
    float fov;
    int phase;
    };


struct PathKey
    {
    float t;
    fVec3 p;
    };


struct CameraKey
    {
    float t;
    fVec3 eye_offset;
    fVec3 target_offset;
    float fov;
    };


// The hero ship flies through one continuous path.  These points are not
// camera cuts; they are the actual flight plan.
static const PathKey SHIP_KEYS[] =
    {
    {   0.0f, {  0.0f,  0.10f,  18.0f } },
    {   6.0f, {  0.0f, -0.05f,  10.0f } },
    {  14.0f, {  0.3f, -0.18f,   0.0f } },
    {  30.0f, {  2.9f,  0.15f, -12.0f } },
    {  42.0f, { -1.8f, -0.22f, -24.0f } },
    {  52.0f, {  2.0f,  0.34f, -34.0f } },
    {  62.0f, { -1.7f, -0.20f, -44.0f } },
    {  72.0f, { -4.9f,  0.35f, -48.4f } },
    {  77.0f, { -0.6f,  0.55f, -56.2f } },
    {  82.0f, {  4.7f, -0.15f, -53.1f } },
    {  94.0f, {  0.4f,  0.00f, -66.0f } },
    { 108.0f, {  0.1f, -0.08f, -82.0f } },
    { 120.0f, {  0.0f,  0.00f,-128.0f } },
    };

static const int NB_SHIP_KEYS = sizeof(SHIP_KEYS) / sizeof(SHIP_KEYS[0]);


// Offsets are expressed in the local ship frame:
// x = right wing direction, y = up, z = forward.
static const CameraKey CAMERA_KEYS[] =
    {
    {   0.0f, { -7.0f,  2.6f, -12.0f }, {  0.0f, 0.0f,  6.0f }, 49.0f },
    {  10.0f, { -2.0f,  1.4f,  -8.0f }, {  0.0f, 0.0f,  4.5f }, 56.0f },
    {  19.0f, {  6.0f,  1.9f,  -3.0f }, { -0.2f, 0.0f,  2.5f }, 57.0f },
    {  44.0f, {  1.5f,  1.0f,  -5.0f }, {  0.0f, 0.0f,  4.5f }, 62.0f },
    {  55.0f, {  0.7f,  1.0f,  -5.3f }, {  0.0f, 0.0f,  5.8f }, 65.0f },
    {  63.0f, { -1.0f,  1.2f,  -5.8f }, {  0.1f, 0.1f,  5.5f }, 63.0f },
    {  76.0f, {  6.8f,  3.0f,  -3.2f }, {  0.0f, 0.1f,  2.8f }, 56.0f },
    {  90.0f, {  0.0f,  2.1f,  -8.2f }, {  0.0f, 0.0f,  7.0f }, 58.0f },
    { 105.0f, { -2.2f,  1.3f,  -6.5f }, {  0.0f, 0.0f,  6.0f }, 61.0f },
    { 120.0f, {  0.0f,  4.0f, -17.0f }, {  0.0f, 0.2f, 32.0f }, 46.0f },
    };

static const int NB_CAMERA_KEYS = sizeof(CAMERA_KEYS) / sizeof(CAMERA_KEYS[0]);


int keySegment(float t, int nb, const PathKey* keys)
    {
    if (t <= keys[0].t) return 0;
    for (int i = 0; i < nb - 1; i++)
        {
        if (t <= keys[i + 1].t) return i;
        }
    return nb - 2;
    }


int cameraSegment(float t)
    {
    if (t <= CAMERA_KEYS[0].t) return 0;
    for (int i = 0; i < NB_CAMERA_KEYS - 1; i++)
        {
        if (t <= CAMERA_KEYS[i + 1].t) return i;
        }
    return NB_CAMERA_KEYS - 2;
    }


fVec3 sampleShipPath(float t)
    {
    // Analytic flight path.  The forward coordinate is monotone, so the ship
    // never stops or reverses at shot boundaries.  Lateral manoeuvres are
    // blended in/out with smooth envelopes.
    const float final_boost = smoothRange(t, 114.0f, 120.0f);
    const float start_far = 95.0f * (1.0f - smoothRange(t, 0.0f, 10.0f));
    const float z = 18.0f + start_far - 0.82f * t - 0.0015f * t * t - 190.0f * final_boost * final_boost * final_boost;

    float x = 0.30f * sinf(t * 0.075f + 0.4f);
    float y = -0.18f + 0.16f * sinf(t * 0.052f + 0.7f);

    const float wide = fadeInOut(t, 9.5f, 13.0f, 21.0f, 27.0f);
    x += 1.05f * sinf((t - 10.0f) * 0.11f) * wide;
    y += 0.12f * sinf((t - 10.0f) * 0.08f) * wide;

    const float slalom = fadeInOut(t, 50.0f, 56.0f, 68.0f, 74.0f);
    x += 1.45f * sinf((t - 50.0f) * 0.70f) * slalom;
    y += 0.34f * sinf((t - 50.0f) * 1.08f + 0.4f) * slalom;

    const float evade = fadeInOut(t, 100.0f, 103.0f, 110.0f, 113.0f);
    x += 0.32f * sinf((t - 100.0f) * 0.70f) * evade;
    y += 0.72f * sinf((t - 100.0f) * 0.92f) * evade;

    const float orbit = fadeInOut(t, 72.0f, 76.0f, 88.0f, 96.0f);
    const float ou = smoothRange(t, 72.0f, 96.0f);
    const float a = 1.55f + 3.35f * ou;
    const fVec3 center = { 0.6f, -0.8f, -58.0f };
    const fVec3 orbit_p = { center.x + 5.25f * cosf(a),
                            center.y + 1.15f + 0.28f * sinf(a * 0.7f),
                            center.z + 5.25f * sinf(a) };

    return mixv({ x, y, z }, orbit_p, orbit);
    }


fVec3 sampleShipVelocity(float t)
    {
    const float dt = 0.12f;
    const fVec3 a = sampleShipPath(clampf(t - dt, 0.0f, MOVIE_SECONDS));
    const fVec3 b = sampleShipPath(clampf(t + dt, 0.0f, MOVIE_SECONDS));
    return normv(b - a);
    }


ShipState computeShipState(float t)
    {
    ShipState s;
    s.p = sampleShipPath(t);
    s.v = sampleShipVelocity(t);
    s.scale = 1.52f;

    const fVec3 before = sampleShipVelocity(clampf(t - 0.35f, 0.0f, MOVIE_SECONDS));
    const fVec3 after = sampleShipVelocity(clampf(t + 0.35f, 0.0f, MOVIE_SECONDS));
    const float turn = clampf((after.x - before.x) * 9.0f, -1.0f, 1.0f);
    const float climb = clampf(s.v.y * 4.0f, -1.0f, 1.0f);

    const float slalom = fadeInOut(t, 48.0f, 54.0f, 68.0f, 75.0f);
    const float barrel = fadeInOut(t, 42.0f, 44.0f, 47.0f, 50.0f);
    const float formation = smooth01((t - 88.0f) / 18.0f);

    s.roll = -22.0f * turn + 18.0f * slalom * sinf((t - 44.0f) * 0.70f);
    s.roll += 360.0f * barrel;
    s.roll *= 1.0f - 0.55f * formation;
    s.pitch = (-1.0f + 2.0f * climb) * (1.0f - 0.75f * formation);
    return s;
    }


void sampleCameraOffset(float t, fVec3& eye_offset, fVec3& target_offset, float& fov)
    {
    const int i = cameraSegment(t);
    const int i0 = (i > 0) ? i - 1 : i;
    const int i3 = (i + 2 < NB_CAMERA_KEYS) ? i + 2 : i + 1;
    const float span = CAMERA_KEYS[i + 1].t - CAMERA_KEYS[i].t;
    const float u = smoother01((t - CAMERA_KEYS[i].t) / span);

    eye_offset = catmullRom(CAMERA_KEYS[i0].eye_offset, CAMERA_KEYS[i].eye_offset,
                            CAMERA_KEYS[i + 1].eye_offset, CAMERA_KEYS[i3].eye_offset, u);
    target_offset = catmullRom(CAMERA_KEYS[i0].target_offset, CAMERA_KEYS[i].target_offset,
                               CAMERA_KEYS[i + 1].target_offset, CAMERA_KEYS[i3].target_offset, u);
    fov = mixf(CAMERA_KEYS[i].fov, CAMERA_KEYS[i + 1].fov, u);
    }


fVec3 localToWorld(const ShipState& ship, const fVec3& local)
    {
    const CameraBasis b = makeShipBasis(ship.v);
    return ship.p + b.right * local.x + b.up * local.y + b.forward * local.z;
    }


ShipState formationAnchor(float t, const ShipState& ship)
    {
    ShipState anchor;
    anchor.p = sampleShipPath(clampf(t - 0.9f, 0.0f, MOVIE_SECONDS));
    anchor.v = normv(sampleShipVelocity(clampf(t - 2.0f, 0.0f, MOVIE_SECONDS)) +
                     sampleShipVelocity(clampf(t + 2.0f, 0.0f, MOVIE_SECONDS)));
    anchor.scale = ship.scale;
    anchor.roll = 0.0f;
    anchor.pitch = 0.0f;
    return anchor;
    }


CameraState formationCamera(float t, const ShipState& ship)
    {
    CameraState c;
    const float u = smoother01((t - 96.0f) / 16.0f);
    const float close = sinf(MOVIE_PI * u);
    const float tie_close = smooth01((u - 0.45f) / 0.16f) * (1.0f - smooth01((u - 0.61f) / 0.17f));
    const ShipState anchor = formationAnchor(t, ship);
    const CameraBasis ab = makeShipBasis(anchor.v);

    const float eye_distance = 11.5f - 4.8f * close - 1.45f * tie_close;
    const float target_distance = 3.6f - 1.05f * close - 0.15f * tie_close;
    c.eye = anchor.p - ab.forward * eye_distance + ab.up * (0.98f + 0.15f * close + 0.04f * tie_close);
    c.target = anchor.p - ab.forward * target_distance + ab.up * (0.72f + 0.08f * tie_close);
    c.fov = 59.0f + 6.0f * close + 3.0f * tie_close;
    c.phase = 5;
    return c;
    }


CameraState computeCameraState(float t, const ShipState& ship)
    {
    CameraState c;
    const CameraBasis b = makeShipBasis(ship.v);
    fVec3 eye_offset = { 0.0f, 1.2f, -6.0f };
    fVec3 target_offset = { 0.0f, 0.05f, 5.5f };
    c.fov = 58.0f;

    if (t < 50.0f)
        {
        // One continuous opening shot:
        //   1. rush in from very far away,
        //   2. follow the ship into the asteroid field,
        //   3. orbit once around the ship while keeping it large on screen,
        //   4. settle smoothly on the wing camera used by the slalom.
        const fVec3 e_far     = { -0.35f, 2.85f, -360.0f };
        const fVec3 e_wing    = {  1.38f, 0.54f,   -1.72f };
        const fVec3 t_far     = {  0.00f, 0.40f,   60.00f };
        const fVec3 t_center  = {  0.00f, 0.04f,    0.25f };
        const fVec3 t_wing    = {  0.00f, 0.04f,    3.85f };
        const float orbit_start = 10.0f;
        const float orbit_end = 39.5f;
        const float orbit_len = orbit_end - orbit_start;
        const float a0 = -2.10f;
        const float r0 = 1.95f + 0.55f;
        const fVec3 e_orbit_start = { r0 * sinf(a0), 0.58f, r0 * cosf(a0) };
        const fVec3 orbit_tangent = { r0 * cosf(a0) * (2.0f * MOVIE_PI) * orbit_start / orbit_len,
                                      0.18f * MOVIE_PI * orbit_start / orbit_len,
                                     -r0 * sinf(a0) * (2.0f * MOVIE_PI) * orbit_start / orbit_len };
        const fVec3 target_tangent = { 0.0f, 0.0f, 0.0f };

        if (t < orbit_start)
            {
            const float u = clampf(t / orbit_start, 0.0f, 1.0f);
            eye_offset = hermitev(e_far, e_orbit_start, { 0.0f, 0.0f, 0.0f }, orbit_tangent, u);
            target_offset = hermitev(t_far, t_center, { 0.0f, 0.0f, 0.0f }, target_tangent, u);
            c.fov = hermitef(24.0f, 76.0f, 0.0f, 0.0f, u);
            }
        else if (t < orbit_end)
            {
            const float q = (t - orbit_start) / orbit_len;
            const float a = -2.10f + 2.0f * MOVIE_PI * q;
            const float radius = 1.95f + 0.55f * (0.5f + 0.5f * cosf(2.0f * MOVIE_PI * q));
            eye_offset = { radius * sinf(a),
                           0.58f + 0.18f * sinf(MOVIE_PI * q),
                           radius * cosf(a) };
            target_offset = t_center;
            c.fov = 76.0f - 5.0f * sinf(MOVIE_PI * q);
            }
        else
            {
            const float u = smoother01((t - orbit_end) / 10.5f);
            const float a_end = -2.10f + 2.0f * MOVIE_PI;
            const float r_end = 1.95f + 0.55f;
            const fVec3 e_orbit_end = { r_end * sinf(a_end), 0.58f, r_end * cosf(a_end) };
            eye_offset = mixv(e_orbit_end, e_wing, u);
            target_offset = mixv(t_center, t_wing, u);
            c.fov = mixf(76.0f, 63.0f, u);
            }
        }
    else if (t < 72.0f)
        {
        const float u = smoother01((t - 50.0f) / 22.0f);
        const float pullback = smooth01((u - 0.34f) / 0.66f);
        const float close = sinf(MOVIE_PI * clampf(u / 0.45f, 0.0f, 1.0f));
        eye_offset = mixv({ 1.38f, 0.54f, -1.72f }, { 1.15f, 0.88f, -4.95f }, pullback);
        eye_offset.x += 0.22f * sinf(MOVIE_PI * u);
        eye_offset.y -= 0.12f * close;
        target_offset = mixv({ 0.0f, 0.04f, 3.85f }, { 0.0f, 0.0f, 6.0f }, pullback);
        c.fov = 63.0f + 4.0f * close;
        }
    else if (t < 96.0f)
        {
        // Rock orbit shot: the camera orbits the central asteroid while still
        // following the ship.  This makes the large rock read as a true 3D
        // object instead of a distant sprite.
        const float u0 = smooth01((t - 72.0f) / 24.0f);
        const float fast_u = powf(u0, 0.48f);
        const float ease_to_end = smoother01((u0 - 0.58f) / 0.42f);
        const float u = mixf(fast_u, u0, ease_to_end);
        const fVec3 center = { 0.6f, -0.8f, -58.0f };
        const float a = 0.7f + 3.2f * u;
        c.eye = center + fVec3{ 8.8f * sinf(a), 2.4f + 0.9f * sinf(MOVIE_PI * u), 8.8f * cosf(a) };
        c.target = mixv(center, ship.p, 0.45f);
        c.fov = 54.0f;
        c.phase = 4;
        return c;
        }
    else if (t < 112.0f)
        {
        return formationCamera(t, ship);
        }
    else
        {
        if (t < 116.0f)
            {
            const float v = smoother01((t - 112.0f) / 4.0f);
            const ShipState ref_ship = computeShipState(112.0f);
            const CameraState start = formationCamera(112.0f, ref_ship);
            c.eye = mixv(start.eye, localToWorld(ship, { 0.25f, 0.70f, -1.42f }), v);
            c.target = mixv(start.target, localToWorld(ship, { 0.0f, 0.0f, 4.4f }), v);
            c.fov = mixf(start.fov, 74.0f, v);
            c.phase = 6;
            return c;
            }
        else
            {
            const float v = smoother01((t - 116.0f) / 4.0f);
            ShipState ref;
            ref.p = sampleShipPath(116.0f);
            ref.v = sampleShipVelocity(116.0f);
            ref.scale = ship.scale;
            ref.roll = ship.roll;
            ref.pitch = ship.pitch;
            const CameraBasis rb = makeShipBasis(ref.v);
            c.eye = ref.p + rb.right * 0.25f + rb.up * 0.70f - rb.forward * 1.42f;
            c.target = ref.p + rb.forward * mixf(4.4f, 130.0f, v) + rb.up * 0.05f;
            c.fov = mixf(74.0f, 31.0f, v);
            c.phase = 6;
            return c;
            }
        }

    c.eye = localToWorld(ship, eye_offset);
    c.target = localToWorld(ship, target_offset);

    if (t < 10.0f) c.phase = 0;
    else if (t < 13.0f) c.phase = 1;
    else if (t < 50.0f) c.phase = 2;
    else if (t < 72.0f) c.phase = 3;
    else if (t < 96.0f) c.phase = 4;
    else if (t < 112.0f) c.phase = 5;
    else c.phase = 6;
    return c;
    }


const char* phaseName(int phase)
    {
    switch (phase)
        {
        case 0: return "approach";
        case 1: return "wide fly-by";
        case 2: return "camera sweep";
        case 3: return "slalom";
        case 4: return "rock orbit";
        case 5: return "formation";
        default: return "escape";
        }
    }
