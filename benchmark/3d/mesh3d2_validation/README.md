# Mesh3D2 CPU Validation

This validation compares `Renderer3D::drawMesh(Mesh3D)` against
`Renderer3D::drawMesh(Mesh3D2)` on several generated OBJ models and view angles.

The reference `Mesh3D` face stream is generated with the same triangle strips as
`Mesh3D2`, so the no-culling images must match pixel-for-pixel. The culling check then
compares `Mesh3D2` with visibility cones disabled against `Mesh3D2` with cones enabled.
The render uses a flat constant material on purpose: this validates visible pixel
coverage exactly, without reporting harmless Gouraud/z-tie shading changes from hidden
triangles that are no longer drawn.

Run from PowerShell:

```powershell
benchmark\3d\mesh3d2_validation\run.ps1
```

The script regenerates temporary headers in `generated/`, builds a CPU executable and
writes `out/mesh3d2_validation.csv`. Debug BMPs are written only for failing views.
