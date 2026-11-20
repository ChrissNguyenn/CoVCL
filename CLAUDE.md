# CoVCL: notes for Claude Code

CoVCL is a EuroScope plugin (C++ MFC DLL, x86) for VCLvACC (Vietnam VVHN/VVHM, Cambodia VDPF, Laos VLVT),
forked from CoFrance (GPL v3). Keep the license and the credit to Pierre Ferran.

## Structure

| File | What it does |
|---|---|
| `CoVCL.cpp/.h`, `CoVCL.def` | MFC DLL entry point, `EuroScopePlugInInit` creates the plugin |
| `CoVCLPlugIn.cpp/.h` | Main plugin: registers tag items and functions, `OnGetTagItem` (all tag items), `OnCompileCommand` (`.covcl ...`), `OnTimer` (STCA every 3 s, OCL poll), `LoadConfigFile` (web, then local `CoVCL.toml`) |
| `RadarScreen.cpp/.h` | Radar screen: menu bar buttons (RAD, FILTERS, L/U levels, VV, APP, DYP), sector ceiling labels, aircraft symbols and trails, separation tool, filters |
| `STCA.cpp/.h` | Short term conflict alert, thresholds from `[stca]` in config |
| `DYPWindow.h` | DYP flight info window (MAIN / ROUTE / OCL tabs) |
| `PopupSpeedAssign.h` | ASP speed assignment popup |
| `VERA.h` | Closest point of approach maths for the separation tool |
| `Constants.h` | Tag item IDs, symbols, static colours, helpers (`startsWith`, `padWithZeros`...), online config URL, oceanic OCL helpers |
| `config.toml` | Colours, STCA, menu, `[[arrival_runway_colour]]` (VVTS, VVNB), generated `[[sector_vertical_limit]]` labels |
| `tools/generate_sector_labels.py` | Regenerates the ceiling labels from the VCL `.ese` (`--write-config config.toml`) |
| `lib/` | EuroScope SDK header and import lib (not GPL) |
| `.github/workflows/` | Windows build with MSVC + vcpkg (openssl, curl, nlohmann-json, cpp-httplib, toml11) |

## Rules

- The code cannot be compiled on Linux/macOS: it needs MSVC + MFC. Verify by pushing and reading the GitHub Actions log.
- Position IDs in config (`controller_me`, `controller_others`, `controller_not_others`) are the short IDs from the `.ese` `[POSITIONS]` section (field 4), e.g. `VVHN`, `HN1`, `VHS3`, `HCMA`, `PP`, `VLVT`, `VCL`, `NBTE`, `TSTE`.
- New tag items: add an ID in `CoVCLTags` (Constants.h), `RegisterTagItemType` in the plugin constructor, and a branch in `OnGetTagItem`.
- New config keys: read them with `toml::find`, and guard optional ones with `.contains()` so old config files keep loading.
- `CoVCL.rc` is UTF-16: edit it with a tool that keeps the encoding, and keep it ASCII-only (CI rewrites it as UTF-8 without BOM).
- `Constants.h` is UTF-8 **with BOM**: the BOM makes MSVC encode the glyph strings (`±§÷µ©®ß`) as single cp1252 bytes for the
  French EuroScope font. Never strip it. `RadarScreen.cpp` is Windows-1252 (`°`).
- CI runs on `windows-2022` (VS 2022, v143 + MFC). `windows-latest` (VS 2026) has no v143 MFC; moving to it means switching
  the project to the v145 toolset.
- The online config URL in `Constants.h` points at the `master` branch of ChrissNguyenn/CoVCL: keep pushing to `master`.
