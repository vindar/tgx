# TGX 2D Test Coverage Plan

This file tracks the broad validation/benchmark suite coverage.

## Current Broad Suite Coverage

CPU:

- `RGB24`, `RGB32`, `RGBf`
- 800x600 output images
- BMP snapshots and CSV timings/hashes
- stable CSV schema and baseline comparison via `benchmark/2d/baselines/cpu_hashes.csv`
- optional golden-image comparison against archived BMP snapshots in `benchmark/2d/golden/cpu`, with diagnostic diff BMPs on mismatch
- extra BMP snapshots for state/copy/fill/spline-heavy groups
- optional `-Large` desktop pass with 2048x2048 default output, configurable with `-LargeSize`, and a separate baseline at `benchmark/2d/baselines/cpu_large_hashes.csv`

Teensy 4.1:

- `RGB565`
- 320x240 ILI9341 display pages
- USB Serial timings/hashes; the sketch waits for one received character before starting
- machine-readable `RESULT` lines, CSV extraction and baseline comparison via `benchmark/2d/baselines/teensy4_hashes.csv`
- separate optional-integration sketch for `PNGdec`, `JPEGDEC`, `AnimatedGIF` and `OpenFontRender`, with baseline comparison via `benchmark/2d/baselines/teensy4_optional_hashes.csv`

Covered method families in the first broad suite:

- image setup and state: default construction, `set()`, `setInvalid()`, `isValid()`, `lx()`, `ly()`, `stride()`
- image attributes and buffer access: `width()`, `height()`, `dim()`, `imageBox()`, const/non-const `data()`
- image edge cases: null buffers, invalid stride, fully-outside crops, nested crops, clipped `iterate()` boxes, `copyReduceHalf()` odd dimensions, one-row and one-column paths, too-small destination failure
- direct pixels: `drawPixel()`, `drawPixelf()`, `readPixel()`, `readPixelf()`, `operator()`
- exact primitive checks: out-of-range `drawPixel()`, endpoint pixels/counts for `drawFastHLine()`, `drawFastVLine()`, horizontal/diagonal/reversed/single-point `drawLine()`, `drawSegment()` endpoint flags, clipped lines, one-row/one-column `drawRect()`, tiny `drawThickRect()` fill fallback, clipped `fillRect()`, `fillThickRect()`
- sub-images and cropping: `operator()`, `getCrop()`, `crop()`, shared-buffer behavior, `fillScreen()` on cropped images
- iteration: `iterate()`, const `iterate()`, `iterate(box)`, early-stop callback behavior
- fills, gradients and opacity: `clear()`, `fillScreen()`, `fillRect()`, `fillRectHGradient()`, `fillRectVGradient()`, `fillScreenHGradient()`, `fillScreenVGradient()`, simple opacity `0.0` and `1.0` checks
- flood fill: both `fill()` variants, including border fill and unicolor component fill
- flood fill tiny-stack failure path and `-1` return
- copy/convert/reduce: `copyFrom()`, custom `copyFrom()`/`blit()` operator, `copyReduceHalf()`, `reduceHalf()`, `convert()`, `blitBackward()`
- lines: `drawFastHLine()`, `drawFastVLine()`, `drawLine()`, `drawSegment()`, AA footprint/clipping/opacity checks for `drawLineAA()`, `drawThickLineAA()`, `drawWedgeLineAA()`, plus deprecated wrappers `drawWideLine()`, `drawWedgeLine()`, `drawSpot()`
- rectangles: `drawRect()`, `drawThickRect()`, `fillRect()`, `fillThickRect()`, `fillRectHGradient()`, `fillRectVGradient()`, AA footprint/interior/opacity checks for `drawThickRectAA()`, `fillRectAA()`, `fillThickRectAA()`, exact checks for `drawRoundRect()` and `fillRoundRect()`, AA footprint/interior/opacity checks for `drawRoundRectAA()`, `drawThickRoundRectAA()`, `fillRoundRectAA()`, `fillThickRoundRectAA()`
- circles/arcs/ellipses: exact cardinal/interior/outline/clipping/opacity checks for `drawCircle()`, `fillCircle()`, `drawEllipse()`, `fillEllipse()`, AA footprint/interior/opacity checks for `drawCircleAA()`, `drawThickCircleAA()`, `fillCircleAA()`, `fillThickCircleAA()`, `drawEllipseAA()`, `fillEllipseAA()`, `fillThickEllipseAA()`, plus broad coverage for `drawCircleArcAA()`, `drawThickCircleArcAA()`, `fillCircleSectorAA()`, `fillThickCircleSectorAA()`, `drawThickEllipseAA()`
- rounded rectangles: exact center/edge/corner/opacity checks for `drawRoundRect()` and `fillRoundRect()`, plus AA footprint/interior/opacity checks for AA rounded-rectangle variants
- triangles/quads: exact corner/interior/orientation/clipping checks for `drawTriangle()`, `fillTriangle()`, `drawQuad()`, `fillQuad()`, AA footprint/interior/opacity checks for `drawTriangleAA()`, `drawThickTriangleAA()`, `fillTriangleAA()`, `fillThickTriangleAA()`, `drawQuadAA()`, `drawThickQuadAA()`, `fillQuadAA()`, `fillThickQuadAA()`, plus broad coverage for `drawGradientTriangle()` and `drawGradientQuad()`
- polygons/splines: exact and broad checks for array/functor polyline and polygon paths, including the TGX functor contract where the last point is stored before returning `false`; `drawPolyline()`, `drawPolylineAA()`, `drawThickPolylineAA()`, `drawPolygon()`, `fillPolygon()`, `drawPolygonAA()`, `drawThickPolygonAA()`, `fillPolygonAA()`, `fillThickPolygonAA()`, `drawQuadBezier()`, `drawCubicBezier()`, `drawQuadSpline()`, `drawCubicSpline()`, `drawClosedSpline()`, `drawThickQuadBezierAA()`, `drawThickCubicBezierAA()`, `drawThickQuadSplineAA()`, `drawThickCubicSplineAA()`, `drawThickClosedSplineAA()`, `fillClosedSplineAA()`, `fillThickClosedSplineAA()`, clipping/footprint checks and opacity `0.0`
- blit/transform: `blit()`, custom `blit()` operator, `blitMasked()`, `blitRotated()` with exact 0/90/180/270 pixel mapping, custom `blitRotated()` operator, `blitScaledRotated()`, `blitScaledRotatedMasked()`, `copyFrom()`
- visual ops snapshot extension: bottom panels for scaled/rotated/masked blits, custom blend operators, clipped textured triangles/quads and textured gradient paths
- clipping paths: clipped horizontal/vertical lines, clipped negative-position `blit()`, clipped `blitMasked()` with transparent texels, same-color custom blit operator
- text: `fontHeight()`, `measureChar()`, `drawChar()`, `drawText()`, `drawTextEx()`, `measureText()`, baseline/top/bottom/left/right/center anchor placement, wrapping height, newline cursor advancement, `start_newline_at_0`, wrap-to-zero behavior near the right edge, opacity `0.0`, deprecated measure/draw overloads with pixel-equivalence checks, ILI and Adafruit `GFXfont` paths, longer wrapped paragraphs, bottom-right clipping, mixed ILI/GFX font sequences, and multiline GFX stress
- textured primitives: triangle and quad variants including textured, gradient, masked, gradient-masked and custom blend-operator paths; exact edge checks for clipping, invalid sources, full and partial masks, opacity `0.0`, and quad-to-two-triangles decomposition
- large-frame CPU stress/performance: gradients, dense line/circle/AA primitive overlays, blits and textured primitive batches, wrapped text/GFX text, and a full layered 2048x2048 scene for `RGB24`, `RGB32` and `RGBf`
- optional Arduino integrations on Teensy: `PNGDecode()` with `PNGdec`/`TGX_PNGDraw`, `JPEGDecode()` with `JPEGDEC`/`TGX_JPEGDraw`, `GIFplayFrame()` with `AnimatedGIF`/`TGX_GIFDraw`, and `setOpenFontRender()` callbacks with opacity `0.0` validation
