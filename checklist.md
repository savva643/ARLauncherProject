# ARLauncher Vulkan Migration Checklist (wlroots path)

## Constraints / Invariants
- [ ] Do not modify `sensor connector` data acquisition (video/depth/mesh/pose).
- [ ] Keep pose timestamps and coordinate conventions consistent end-to-end.
- [ ] Prefer incremental changes: Vulkan must reach feature parity with current OpenGL path before removing OpenGL.

## Phase 0 — Repo / Build Readiness
- [ ] Confirm current build options/macros: `USE_OPENGL`, `USE_VULKAN`, `USE_SENSOR_CONNECTOR`.
- [ ] Ensure Vulkan SDK availability and minimal Vulkan compile/link works in CI/local.
- [ ] Decide Vulkan integration approach:
  - [ ] Option A: integrate The Forge (recommended for fast iteration)
  - [ ] Option B: keep custom Vulkan backend and only borrow pieces (ImGui, asset loading)
- [ ] Add a clear build preset(s):
  - [ ] `-DUSE_VULKAN=ON -DUSE_OPENGL=OFF` (target)
  - [ ] `-DUSE_OPENGL=ON` (fallback during migration)

## Phase 1 — Vulkan Baseline (Window + Swapchain + Render Loop)
- [ ] Switch `Application::initializeRenderer()` to allow Vulkan selection (feature-flag).
- [ ] Vulkan renderer baseline:
  - [ ] Instance/device/swapchain creation (already partially exists)
  - [ ] Robust resize handling (`VK_ERROR_OUT_OF_DATE_KHR`)
  - [ ] Command buffers per swapchain image
  - [ ] Synchronization (semaphores/fences)
- [ ] Basic graphics pipeline:
  - [ ] Vertex + fragment shaders (unlit color)
  - [ ] Vertex input for current mesh format: `pos(3) normal(3) uv(2)`
  - [ ] Uniforms/push constants for `model`, `view`, `projection`
  - [ ] `vkCmdDrawIndexed` path

## Phase 2 — Virtual Camera (Pose -> View/Projection)
- [ ] Verify pose ingestion path (no changes to sensor connector):
  - [ ] Pose callback updates target pose
  - [ ] Per-frame interpolation (already in `Application::update()` under `USE_SENSOR_CONNECTOR`)
  - [ ] `Scene::updateCameraFromAR(position, rotation)`
- [ ] Confirm view matrix correctness:
  - [ ] Using conjugate quaternion + negative translation (already in `Camera`/`CameraController`)
  - [ ] Validate axes (ARKit vs engine): forward/up/right expectations
- [ ] Projection:
  - [ ] Start with static FOV + aspect
  - [ ] Later: use iPhone intrinsics/FOV once available from sensor connector

## Phase 3 — Test Geometry (Cube + Sphere + Torus)
- [ ] Keep demo scene minimal and deterministic:
  - [ ] Cube centered
  - [ ] Sphere left
  - [ ] Torus right
  - [ ] Distance ~1m in front of camera
- [ ] Rotation animation per-object:
  - [ ] Cube: rotate around Y
  - [ ] Sphere: rotate around X+Y
  - [ ] Torus: rotate around Z (or Y)
- [ ] Ensure Vulkan path renders the same meshes as OpenGL path.

## Phase 4 — iPhone Video Background
- [ ] Texture update strategy:
  - [ ] Stage buffer / linear image update
  - [ ] Convert/accept format (BGRA/NV12) as produced by sensor connector
- [ ] Fullscreen background pass:
  - [ ] Option 1: draw fullscreen quad sampling video texture
  - [ ] Option 2: blit/copy into swapchain (current Vulkan code copies image -> swapchain; validate correctness)
- [ ] Latency:
  - [ ] Keep 1-2 frame buffering max

## Phase 5 — Depth / Occlusion
- [ ] Depth-map import:
  - [ ] Create depth texture (e.g. `R16_SFLOAT`)
  - [ ] Upload each frame
- [ ] Occlusion approaches:
  - [ ] A: pre-fill depth buffer from depth-map (fullscreen depth write)
  - [ ] B: render LiDAR mesh into depth-only pass
- [ ] Validate depth units (meters) and near/far planes.

## Phase 6 — LiDAR Mesh
- [ ] Mesh upload/update:
  - [ ] Dynamic vertex/index buffers
  - [ ] Update at ~1–5 Hz
- [ ] Rendering modes:
  - [ ] Depth-only (occlusion)
  - [ ] Debug visualization (wireframe/solid)

## Phase 7 — UI in 3D (ImGui -> Texture -> Quad)
- [ ] Integrate ImGui for Vulkan.
- [ ] Render ImGui into an offscreen texture every frame.
- [ ] Place that texture on a 3D quad (billboard optional).
- [ ] Future input mapping:
  - [ ] Ray -> UV -> ImGui events

## Phase 8 — wlroots Compositor Integration (Wayland windows as 3D surfaces)
- [ ] Start from `tinywl`-like minimal compositor.
- [ ] Surface capture:
  - [ ] Export client surface textures as DMA-BUF
  - [ ] Import DMA-BUF into Vulkan images
- [ ] Scene representation:
  - [ ] Each window = textured quad in 3D
  - [ ] Z-order / focus -> depth/placement rules

## Phase 9 — Cleanup / Removal of OpenGL
- [ ] Ensure Vulkan reaches parity for:
  - [ ] background video
  - [ ] 3D meshes
  - [ ] UI panel
- [ ] Remove OpenGL-only dependencies (`NVG`, OpenGL shaders) only after parity.

## Acceptance Checklist (Milestone)
- [ ] Virtual camera follows real iPhone pose (position + rotation) without jitter.
- [ ] Three test objects (cube, sphere, torus) spawn and rotate in correct positions.
- [ ] iPhone video appears as background.
- [ ] Depth-based occlusion works (real depth hides virtual objects).
- [ ] At least one Wayland client window is shown as a 3D quad.
