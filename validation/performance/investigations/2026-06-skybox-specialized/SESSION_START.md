# TGX Specialized Skybox Session Start

Date: 2026-06-07

Commit: `c0f935dfcf02c8f6096af916475c156122dec450`

Initial source diff: clean.

## Goal

Add and evaluate a first specialized skybox drawing path for `Renderer3D`.

The current `mars` example renders the skybox with `drawCube()` and regular
3D shader dispatch. This is robust but does more work than needed for a
background cube:

- z-buffer test/write remains active because `setShaders()` does not disable
  the z-buffer selected by `setZbuffer()`;
- flat lighting still computes face color and normal normalization;
- the general `shader_select()` dispatch is used;
- regular clipping includes the far plane.

## Version 1 Strategy

Keep the same world-coordinate cube geometry and the same texture face
conventions as `drawCube()`.

Implement a dedicated `drawSkyBox()` path that:

- uses the current model transform, so callers still scale/position the cube
  with `setModelPosScaleRot()`;
- draws the six named faces: front, back, top, bottom, left, right;
- uses the same default texture coordinates as `drawCube()`;
- uses perspective projection and normal near/screen clipping;
- skips far-plane clipping;
- bypasses `shader_select()` and calls the minimal `uber_shader` variant
  directly;
- uses no z-buffer;
- uses unlit textured shading;
- supports current texture quality/wrap mode when available.

The method must not add `SHADER_NOZBUFFER` to the normal `Renderer3D`
template requirements, because that would add a rarely used branch to the
general shader dispatch for all later triangle rasterization.

## Validation Plan

1. CPU compile and validation.
2. CPU synthetic skybox image/hash/performance comparison against old
   `drawCube()` usage.
3. Teensy 4.1 compile/upload/capture for a small skybox validation sketch.
4. Mars example real-use comparison where feasible.
5. Code size and diff review.

## Hardware

- Teensy 4.1 + ILI9341: `COM3`

The user asked for CPU and Teensy 4 validation for this first version.
