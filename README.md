# CoVCL

EuroScope plugin for VCLvACC controllers (Vietnam, Cambodia, Laos), based on
[CoFrance](https://github.com/vaccfr/CoFrance) by Pierre Ferran and the French vACC.

## Features

All CoFrance features are kept:

- Tag items: 2-digit ground speed, vertical indicator and vertical speed, RFL, CFL, coordinated exit point and level (COPX, read from the sector file), conflict groups, approach intention code, STCA flag, assigned speed (ASP popup), stand (placeholder).
- Radar screen menu: RAD, FILTERS (lower/upper level filter), VV speed vectors, APP mode, DYP window, separation tool.
- STCA (short term conflict alert).
- Sector ceiling labels, generated from the VCL `.ese` file.

## Changes from CoFrance

- Renamed to CoVCL. Commands are `.covcl reload`, `.covcl reload-local` (or `.covrl`), `.covcl perf`, `.covcl debug`.
- The online config is downloaded from this repository: `https://raw.githubusercontent.com/ChrissNguyenn/CoVCL/master/config.toml`
  (`Constants.h`, `CONFIG_ONLINE_URL_PATH`). If it can't be reached, the local `CoVCL.toml` next to the DLL is used.
- The LFPG south-runway colour rule is now a config table, `[[arrival_runway_colour]]`: arrivals to VVTS 07L/25R and
  VVNB 11L/29R get their own intention-code colour.
- Fixed a GDI+ font leak when drawing the sector ceiling labels (one font per label on every background redraw).
- French labels are in English (FILTERS, Lower/Upper Filter, Speed Vectors, MAIN tab, L/U filter buttons).
- The OCL button is hidden: it shows North Atlantic oceanic clearances from natTrak, which VCL traffic never needs. The code is still there.
- The debug log is written to `CoVCL_custom.log` in the EuroScope folder.

## Install

1. Download `CoVCL.dll` and `CoVCL.toml` from the latest GitHub Actions run (Actions tab, artifact "CoVCL") or from a release.
2. Put both files in the same folder, for example next to your sector file.
3. In EuroScope: Other Set > Plug-ins > Load, pick `CoVCL.dll`, and allow it on your radar screens.
4. The plugin uses the font `vCTR Text` for ceiling labels. Install it on Windows if the labels look wrong.

## Updating the sector ceiling labels

When the sector file changes, regenerate the labels:

```
python tools/generate_sector_labels.py path/to/VVXX-....ese --write-config config.toml
```

Add `--include-tma` to also label approach sectors for centre positions. Commit and push: every controller
gets the new labels at the next `.covcl reload`.

## Build

Pushing to GitHub builds the DLL with GitHub Actions (`.github/workflows/build-release.yml`, artifact "CoVCL" with
`CoVCL.dll` + `CoVCL.toml`). Pushing a tag such as `v1.0.0` also creates a draft release.
The build runs on `windows-2022` (Visual Studio 2022, v143 toolset with MFC): the `windows-latest` image ships
Visual Studio 2026 without v143 MFC.
Locally: Visual Studio 2022 with C++ and MFC, vcpkg, x86 Release.

## License

GPL v3, see LICENSE. Original work Copyright (C) 2020 Pierre Ferran. This adaptation must stay open source under the same license.
