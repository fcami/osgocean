# CLAUDE.md

Project rules for Claude Code when working in this repository.

## Project Structure

- **master**: upstream branch. Read-only. Never commit to master.
- **main**: release branch. Created from the tip of master. Commits to main happen only through merge commits from feature/work branches.
- **All work happens in branches.** Branch from main, work in the branch, merge back to main with a merge commit. No direct commits to main.

## Development Principles

### Test first, change second

No code change without a way to verify it. Before modifying behavior, ensure a test exists that pins the current behavior. Before adding a feature, write a test that defines its contract. Tests are the proof that the code works — not comments, not documentation, not visual inspection.

- **CPU tests** (FFT output, spectrum values, normals) run without a GPU. These are the primary safety net.
- **GPU rendering tests** use FBO offscreen rendering. Compare output against golden reference images using PSNR with a tolerance threshold. No exact pixel matching — different GPUs and drivers produce different floating-point results.
- **Golden files** live in `tests/golden/`. CPU goldens are platform-independent. GPU goldens may be per-renderer (hardware vs llvmpipe).
- A test that cannot run (no GPU available) must skip, not fail.

### Refactor aggressively, but only under test coverage

Refactoring means changing structure without changing behavior. Every refactoring must pass the existing tests with identical results. If there are no tests for the code being refactored, write characterization tests first — pin the current output, then restructure.

- Extract distinct responsibilities into separate classes. A class that does simulation AND rendering AND state management is doing too much.
- Prefer composition over inheritance. If two classes share behavior, extract the shared part into a component they both hold — do not create a base class for code sharing.
- Separate policy from mechanism. The decision of WHAT to compute (which spectrum, which foam model, which lighting equation) should be independent of HOW to render it.
- Make dependencies explicit. If a class needs something, take it as a constructor parameter — do not reach into global state or singletons.

### Guard against regressions at every level

- **Simulation regressions:** FFT output for fixed inputs must remain deterministic. Golden files catch drift.
- **Visual regressions:** Rendered output for fixed scene parameters must remain within PSNR threshold of the baseline. Capture baselines before and after each refactoring.
- **Interface regressions:** Once an interface is published (used by code outside its compilation unit), changing its signature is a breaking change. Extend, do not modify.
- **Performance regressions:** If a change is expected to affect performance (FFT computation time, frame rate), measure before and after. Do not introduce unnecessary allocations, copies, or indirection in hot paths.

### Design for replaceability

Each component should be replaceable without modifying its consumers:
- The wave spectrum (Phillips, JONSWAP, TMA) is a strategy — the FFT engine calls it through an interface, not a concrete class.
- The foam model (height threshold, Jacobian) is a strategy — the renderer reads a foam map without knowing how it was produced.
- The lighting model (Blinn-Phong, PBR) is a strategy — the shader calls a lighting function whose implementation can be swapped.
- The FFT implementation is behind a pimpl — keep it that way. If a second FFT backend is added, extract an interface at that point.

When multiple components collaborate, provide a facade — a single entry point with a simplified API that delegates to the components. A facade is not a God class: it owns and configures its components but does not contain their logic.

Do not introduce abstractions speculatively. If there is only one implementation, a concrete class is fine — extract an interface when a second implementation appears, not before. The simplest design that passes the tests is the correct design.

### Static analysis

Run cppcheck and clang-tidy on changed files. Fix bugs (null dereferences, uninitialized variables, resource leaks, undefined behavior). Do not fix style warnings or modernization suggestions in code that is about to be refactored — that is churn, not improvement.

Add static analysis to CI so new code is checked automatically. Do not require zero warnings on the existing codebase — use a "no new warnings" baseline.

### Commits

- One logical change per commit. Refactoring commits must not change behavior. Enhancement commits must not restructure code.
- Commit messages: imperative mood, short subject line. Tag with purpose prefix: `test:`, `refactor:`, `enhance:`, `fix:`, `doc:`.
- Always use: `git commit --signoff`

## Build Commands

```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo .
cmake --build build -j$(nproc)

# Run CPU tests only
ctest --test-dir build -LE gpu

# Run all tests (requires GPU or llvmpipe)
LIBGL_ALWAYS_SOFTWARE=1 ctest --test-dir build

# Run specific test
ctest --test-dir build -R test_name
```
