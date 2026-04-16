# Resource Audit

Audit of runtime resources required by osgOcean and the example app,
cross-referenced against three sources: the git repository (master
branch), osgOcean-Resources-1.0.rar (Internet Archive), and
osgOcean-Resources-1.0.1.rar (Google Code archive).

## Sources

- **git master:** upstream branch as of 2025
- **v1.0 RAR:** `osgOcean-Resources-1.0.rar` from
  `web.archive.org/web/20160807181456/http://osgocean.googlecode.com/files/osgOcean-Resources-1.0.rar`
- **v1.0.1 RAR:** `osgOcean-Resources-1.0.1.rar` from
  `storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/osgocean/osgOcean-Resources-1.0.1.rar`

## Three-Way Comparison

| Resource | git (master) | v1.0 RAR | v1.0.1 RAR | Loads on OSG 3.6? | Action |
|----------|-------------|----------|-----------|-------------------|--------|
| `textures/sea_foam.png` | 62KB 256x256 RGB (newer) | 305KB 512x512 gray+alpha | same as v1.0 | Yes | Keep git — newer, from heightmap commit `e2c716a` |
| `textures/sun_glare.png` | 14KB | same | same | Yes | Already in git |
| `shaders/*` (39 files) | Yes | No | No | Yes | Already in git (source code, not resource archive) |
| `textures/sky_clear/*` (6) | in `demo/` only | Yes (same) | Yes (same) | Yes | Copy from `demo/` to `resources/` |
| `textures/sky_dusk/*` (6) | in `demo/` only | Yes (same) | Yes (same) | Yes | Copy from `demo/` to `resources/` |
| `textures/sky_fair_cloudy/*` (6) | in `demo/` only | Yes (same) | Yes (same) | Yes | Copy from `demo/` to `resources/` |
| `textures/sky_dusk/old/*` (6) | in `demo/` only | Yes | No (cleaned up) | N/A | Skip — superseded in v1.0.1 |
| `island/islands.ive` | in `demo/` (22.7MB, embedded textures) | 8MB (external textures) | same as v1.0 | **Only git version** | Use git — RAR version is old IVE format, unreadable by OSG 3.6 |
| `island/{detail,moss,sand}.png` | in `demo/` only | No | No | N/A | Not needed — embedded in git's islands.ive |
| `boat/boat.3ds` | removed in `fd436e2` | No | 80KB | Untested | Recover from git history `8aeda26` |

## Key Findings

**islands.ive:** The v1.0 and v1.0.1 RAR versions (8MB) use an old IVE
binary format that OSG 3.6.5 cannot read. The git version (22.7MB, in
`demo/island/`) loads successfully — it has textures embedded rather
than referencing external PNG files.

**sea_foam.png:** The git version (256x256 RGB, 62KB) is newer than the
RAR version (512x512 gray+alpha, 305KB). It was updated in commit
`e2c716a` by Quintijn Hendrickx as part of the heightmap/shoreline
foam feature ("New sea foam texture").

**boat.3ds:** Contributed by Dimitrios Filiagos (commit `f3d8436`),
moved to `resources/boat/` in commit `8aeda26`, then removed from
git in commit `fd436e2` ("added it to the resources zip file in
download section"). The file is identical in git history and the
v1.0.1 RAR (sha256 match). Recovered from git history.

**sky cubemaps:** All 18 PNG files (3 presets x 6 faces) are identical
across all three sources. They existed in `demo/textures/` in git
but not in `resources/textures/` where the example app looks for them.

**sky_dusk/old/:** Six old dusk cubemap faces present in v1.0 RAR and
git `demo/`, removed in v1.0.1 RAR. Superseded by the current dusk
cubemap. Not included.
