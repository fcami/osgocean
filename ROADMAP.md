# osgOcean Roadmap

Ordered work items for improving osgOcean. Work is organized in two
parallel tracks: Track A (refactoring, no behavior change) and
Track B (enhancements, visible improvements). Track A items create
the seams that make Track B items safe and simple.

## Sequence 0: Static Analysis Cleanup

Before any behavioral or structural change, fix real bugs found by
static analysis. This ensures the golden files captured in Sequence 1
reflect correct behavior, not latent bugs.

### A0. Run cppcheck and clang-tidy, fix bugs only

Run cppcheck and clang-tidy across the codebase. Triage the results:

**Fix:** null dereferences, uninitialized variables, resource leaks,
undefined behavior, out-of-bounds access, use-after-free. These are
real bugs regardless of any refactoring plan.

**Ignore (for now):** modernization suggestions (NULL to nullptr,
C casts to static_cast, raw pointers to smart pointers), style
warnings, naming conventions. These affect code that will be
refactored or rewritten in Sequences 2-3. Fixing them now produces
churn in diffs that will be overwritten.

**Add to CI:** Configure cppcheck and clang-tidy in CMakeLists.txt
so that new code is checked on every build. Use a "no new warnings"
baseline — do not require zero warnings on existing code.

Each bug fix is one commit with the `fix:` prefix. No structural
changes, no enhancements — pure bug fixes.

---

## Sequence 1: Test Infrastructure

After Sequence 0, the codebase is free of known bugs. Pin the
current behavior so that regressions are caught automatically.

### A1. Characterization tests

**CPU tests (FFT, spectrum, normals):** Create an FFTSimulation with
fixed inputs (wind speed 12 m/s, direction (1,1), grid size 64,
time 0.0), call `computeHeights()`, and compare the output array
against a golden reference file with a floating-point tolerance
(max absolute error < 1e-6). Do the same for `computeDisplacements()`
and for OceanTile's normal computation. These tests require no GPU,
no window, no display. They run everywhere, including headless CI.

**GPU rendering tests:** Use `osg::Camera` with an FBO attachment for
offscreen rendering — no window, no display server needed. Render
the example scene at a known camera position for each preset (CLEAR,
DUSK, CLOUDY), read back pixels via `osg::Image::readPixels()`, and
compare against a reference PNG using PSNR with a threshold (e.g.,
PSNR > 30dB = pass). PSNR with a reasonable threshold catches
regressions (broken shader, missing texture) while tolerating
GPU/driver variation.

**Tooling:**
- Test framework: simple `main()` + assertions, or Catch2 (header-only).
- Image comparison: PSNR comparator (~20 lines of C++).
- Golden files: checked into `tests/golden/`. CPU goldens are
  platform-independent. GPU goldens may be per-renderer (hardware
  vs llvmpipe).
- CI integration: register all tests with CTest. CPU tests run
  unconditionally. GPU tests get `LABELS gpu` so CI can run
  `ctest -LE gpu` on headless machines. A GPU test that cannot
  create an FBO returns exit code 77 (skip), not failure.

**Feature isolation baselines:** Render with each feature toggled
independently — reflections on/off, foam on/off, god rays on/off,
DOF on/off. This provides per-feature reference images for validating
changes to individual subsystems.

---

## Sequence 2: Refactoring

Extract replaceable strategies from hardcoded implementations. Each
step changes structure without changing behavior. Every refactoring
must pass the characterization tests from Sequence 1 with identical
results.

### A2. Extract the spectrum as a strategy (hours)

**Depends on:** A1.

`phillipsSpectrum()` is a private method of `FFTSimulation::Implementation`.
Extract it to a function object behind a simple interface:

```cpp
class WaveSpectrum {
public:
    virtual ~WaveSpectrum() = default;
    virtual float operator()(const osg::Vec2f& K, float windSpeed4,
                             float maxWave, float A) const = 0;
};
```

`PhillipsSpectrum` becomes one implementation. FFTSimulation takes a
`WaveSpectrum` at construction. Zero changes to the FFT machinery.
The characterization tests pass with identical output.

### A3. Extract the foam model (days)

**Depends on:** A1.

The current foam is hardcoded in the fragment shader:

```glsl
if( vVertex.z > osgOcean_FoamCapBottom )
```

Extract the foam decision into a foam texture computed separately.
The shader reads `texture2D(foamMap, uv).r` instead of comparing
vertex height. The CPU writes the same height-threshold values into
the texture that the shader was computing inline — same pixels,
different structure.

This separates what makes foam (policy) from how foam is rendered
(mechanism). The foam computation becomes testable independent of
the shader.

### A4. Extract the lighting model (days)

**Depends on:** A1.

Replace the hardcoded Fresnel and Blinn-Phong blocks in the fragment
shader with a clean lighting function:

```glsl
vec4 computeOceanLighting(vec3 N, vec3 V, vec3 L, float roughness, float F0);
```

The current implementation becomes the body of that function.
Same pixels, different structure. The function lives in a shader
include file, enabling future replacement without touching the
rest of the surface shader.

---

## Sequence 3: Enhancements

Each item changes what the code produces. Items with no dependency
can be done at any time, including in parallel with Sequence 2.
Items with a dependency name the specific refactoring step required.

### B1. FFTW_MEASURE + threading — CLOSED

Implemented, benchmarked, reverted. FFTW_MEASURE provides no
per-frame benefit on Intel / FFTW 3.3.10 at grid sizes 64-512.
The ESTIMATE plan is already near-optimal. Construction overhead
(55-93ms) is not justified.

bench_fft tool added for measurement on other platforms.

Threading (`fftw_init_threads`) remains untested.

### B2. Sub-surface scattering (hours)

**Depends on:** nothing.

~10 lines of GLSL in the vertex shader. Compute wave thickness
from the height field, apply Beer-Lambert transmittance, add a
backlit glow through thin wave crests when looking toward the sun.
Uses data already available (height field, sun direction, view
direction). No architectural change.

### B3. JONSWAP spectrum (hours)

**Depends on:** A2 (spectrum extracted as strategy).

Write `JONSWAPSpectrum` implementing the `WaveSpectrum` interface.
Produces sharper spectral peaks matching real ocean measurements.

```
S(w) = alpha * g^2 / w^5 * exp(-5/4 * (w_p/w)^4) * gamma^r
where r = exp(-(w - w_p)^2 / (2 * sigma^2 * w_p^2))
```

One class, one test, one constructor parameter.

Reference: Hasselmann et al. (1973) "Measurements of Wind-Wave
Growth and Swell Decay during the Joint North Sea Wave Project
(JONSWAP)".

### B4. Jacobian-based foam (days-weeks)

**Depends on:** A3 (foam model extracted).

Compute the Jacobian determinant J of the displacement map alongside
the existing displacement FFT. Where J < threshold, write foam alpha
into the foam texture. Decay foam over time to create trailing foam
streaks. The shader already reads the foam texture (from A3) — no
shader changes needed.

Reference: Dupuy & Bruneton (2012) "Real-time Animation and
Rendering of Ocean Whitecaps".

### B5. PBR surface BRDF (days)

**Depends on:** A4 (lighting model extracted).

Replace the body of `computeOceanLighting()` with Schlick Fresnel
(F0 = 0.02 for water, n=1.333), GGX NDF, and slope-variance
roughness derived from the FFT spectrum. The function signature from
A4 does not change — only the implementation behind it.

Reference: Bruneton & Neyret (2012) "Real-time Realistic Ocean
Lighting using Seamless Transitions from Geometry to BRDF";
Walter et al. (2007) "Microfacet Models for Refraction through
Rough Surfaces".

---

## Sequence 4: Architecture Review

After Sequences 2 and 3, three concrete interfaces exist and three
enhancements have been validated through them. Use this empirical
experience to articulate the target object model for osgOcean.

### A5. Document the target architecture (days)

**Depends on:** A2, A3, A4, and ideally B3, B4, B5.

1. **Validate or revise the domain abstractions.** WaveSpectrum is
   validated by A2/B3. FoamModel is validated by A3/B4. The lighting
   model is validated by A4/B5. What about WaveField (the FFT output
   as a first-class object), OceanCompositor (the multi-pass pipeline
   currently buried in OceanScene), and individual OceanEffects
   (god rays, silt, distortion, DOF, glare currently managed by
   OceanScene directly)? Predict their interfaces from what the
   extractions taught.

2. **Define the collaborations.** Who creates each object? Who holds
   it? Who calls it? Map the producer-consumer relationships.

3. **Decide the inheritance question.** FFTOceanSurface and
   FFTOceanSurfaceVBO do almost the same thing with different GPU
   buffer strategies. Should this be composition (one surface class,
   pluggable geometry backend) rather than a parallel class hierarchy?

4. **Identify the next extractions.** The object model predicts what
   should be extracted next — likely OceanCompositor (the multi-pass
   pipeline out of OceanScene's 1591-line implementation) or WaveField
   (the FFT output as a first-class struct). These become A6, A7, etc.

The output is a documented target architecture that lives alongside
the code — not a spec to implement blindly, but a hypothesis to test
with each subsequent extraction.

---

## Dependency Diagram

```
A0 (static analysis) ── A1 (tests) ─────┬─── A2 (spectrum) ──┬── B3 (JONSWAP)
                                         │                    │
                                         ├─── A3 (foam) ────┬─┤── B4 (Jacobian foam)
                                         │                  │ │
                                         └─── A4 (lighting) ┤ ├── B5 (PBR BRDF)
                                                             │ │
                                                             └─┴── A5 (architecture doc)

(independent)                                  B1 (FFTW_MEASURE)
(independent)                                  B2 (sub-surface scattering)
```

A0 must complete before A1 (golden files must capture bug-free output).
A2, A3, A4 depend on A1 but not on each other. B3, B4, B5 each
depend on one A-step and can proceed before A5. A5 depends on all
three extractions and ideally on the three enhancements. B1 and B2
are free-standing.

## Deferred

Items that are architecturally significant but not yet justified by
a proven bottleneck or user need:

- **GPU compute FFT:** Replaces the CPU FFT + upload path. Do when
  profiling shows the FFT dominates the frame budget.
- **Projected grid:** Replaces the geomipmapping LOD system with
  screen-space projected vertices. Do when the fixed grid is the
  limiting factor for visual quality.
- **Cascaded FFT:** Multiple grids at different spatial scales for
  detail at all distances. Pairs with GPU FFT — do them together.
- **Kelvin wakes:** Ship/object wave interaction. Independent
  subsystem — build as a height-field overlay, do not entangle with
  the FFT simulation.
