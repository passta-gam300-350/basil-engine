# CLI Build Tool

## Table of Contents
- [Overview](#overview)
- [Usage](#usage)
- [Build Configuration](#build-configuration)
- [Architecture](#architecture)
- [Build Pipeline Phases](#build-pipeline-phases)
- [Resource Discovery & On-Demand Import](#resource-discovery--on-demand-import)
- [Material Texture Dependency Resolution](#material-texture-dependency-resolution)
- [Script Compilation](#script-compilation)
- [Output Directory Structure](#output-directory-structure)
- [GUI Build Mode](#gui-build-mode)
- [Key Files](#key-files)

---

## Overview

The editor executable (`editor.exe`) supports a headless CLI build mode that compiles a game project into a standalone distributable package without launching the GUI. When invoked with `-b` and `-o` arguments, the editor skips GLFW/ImGui initialization and runs the build pipeline synchronously.

This enables:
- CI/CD integration for automated builds
- Command-line packaging without a graphics context
- Reproducible builds from a project directory

---

## Usage

```bash
editor.exe -b <project-dir> -o <output-dir>
```

| Argument | Description |
|---|---|
| `-b <project-dir>` | Path to the game project directory |
| `-o <output-dir>` | Path where the build output will be placed |

### Example

```bash
editor.exe -b C:/Projects/MyGame -o C:/BuildOutput
```

This produces `C:/BuildOutput/MyGame/` containing the standalone executable and all runtime assets.

### Exit Codes

| Code | Meaning |
|---|---|
| `0` | Build succeeded |
| `1` | Build failed (missing manifest, compilation errors, I/O failures) |

---

## Build Configuration

The CLI build reads `<project-dir>/build.yaml` for configuration. If the file does not exist, defaults are used.

### build.yaml Fields

```yaml
output_dir: ""                  # Overridden by -o argument
output_name: "MyGame"           # Name of the output folder and executable
icon_relative_path: ""          # Relative path to .ico file within project dir
resource_cleanup: 1             # 0 = none, 1 = minimal
windowing_mode: 0               # 0 = windowed, 1 = fullscreen
window_size:
  width: 1600
  height: 900
```

When `output_name` is empty, the CLI derives it from the project directory name.

### Resource Cleanup Modes

| Mode | Value | Behavior |
|---|---|---|
| `none` | 0 | Copies all imported resources from `.imports/` |
| `minimal` | 1 | Only packages resources referenced by scenes (including material texture dependencies) |

---

## Architecture

### Dual-Mode Entry Point

```
editor.exe
  │
  ├─ CLI mode (-b and -o present)
  │   ├─ No GLFW/ImGui initialization
  │   ├─ No OpenGL context
  │   ├─ Mono runtime initialized for script compilation
  │   └─ Synchronous build via BuildManager::BuildSync()
  │
  └─ GUI mode (no arguments)
      ├─ Full GLFW + OpenGL + ImGui initialization
      ├─ Dual-context multi-threaded rendering
      └─ Async build via BuildManager::BuildAsync()
```

### Thread Model

CLI mode is entirely single-threaded. The `BuildSync()` method runs on the main thread with no window, no OpenGL context, and no render system. Only the Mono runtime is initialized for C# script compilation.

---

## Build Pipeline Phases

The `BuildSync()` pipeline executes these phases in order:

```
[1/6] Compiling scripts
  ├─ Initialize Mono runtime (MonoEntityManager::initialize())
  ├─ Add script search directory: <project>/assets/scripts
  ├─ Set compile output: <output>/data/managed
  └─ MonoManager::StartCompilation() → produces GameAssembly.dll
      └─ On failure: print diagnostics and exit(1)

[2/6] Discovering resources
  ├─ Build descriptor index from <project>/assets/**/*.desc
  ├─ Parse scene_manifest.order → scene YAML files → {guid, type} pairs
  ├─ For each GUID: check .imports/ for existing binary
  ├─ If missing: look up descriptor → import on-demand via ResourceTypeImporterRegistry
  └─ Resolve material → texture dependencies

[3/6] Creating executable
  ├─ Copy application.exe template → <output>/<name>.exe
  ├─ Copy fmod.dll, mono-2.0-sgen.dll
  ├─ Copy shaders/
  ├─ Inject icon (if configured)
  └─ Generate config.yaml (title, fullscreen, resolution)

[4/6] Copying Mono runtime
  └─ Copy lib/mono/ → <output>/lib/mono/

[5/6] Packaging resources
  ├─ Copy scene_manifest.order + scene files
  ├─ Copy BasilEngine.dll, Engine.Bindings.dll → <output>/data/managed/
  ├─ Copy discovered resources → <output>/assets/bin/
  └─ Copy audio files → <output>/assets/

[6/6] Done
```

---

## Resource Discovery & On-Demand Import

### Descriptor Index

The build scans `<project>/assets/` recursively for `.desc` files and builds a `DescriptorIndex` mapping each resource GUID to its descriptor path and importer type:

```
DescriptorIndex = unordered_map<Guid, {desc_path, importer_type}>
```

### On-Demand Import Flow

For each resource GUID discovered from scene files:

1. Check if binary exists in `<project>/.imports/<guid_hex><suffix>`
2. If found → add to package set
3. If not found → look up GUID in `DescriptorIndex`
   - Found → call `ResourceTypeImporterRegistry::Import(importer_type, desc_path)` to generate the binary
   - Not found → print warning, skip resource

This ensures the CLI build works even if the user hasn't manually imported all resources through the editor GUI.

---

## Material Texture Dependency Resolution

Scene files store material GUIDs but not the texture GUIDs those materials reference. Texture references are stored inside the material descriptors themselves (`material.texture_properties`). To support `ResourceCleanUpMode::minimal`, the build resolves these soft dependencies:

### Resolution Logic

When `discover_soft_dependencies` is enabled and a material resource is encountered:

```
material GUID found in scene
  │
  ├─ Locate .desc file for material GUID
  │   ├─ Via DescriptorIndex (CLI path)
  │   └─ Via working_path() heuristic (GUI path)
  │
  ├─ Deserialize: MaterialDescriptor = yaml_serializer::deserialize<MaterialDescriptor>(descPath)
  │
  └─ For each (key, texGuid) in material.texture_properties:
      └─ AddResource({texGuid, "texture"})
```

This ensures that when building with minimal cleanup, only textures actually referenced by materials used in the game's scenes are included in the output — not every texture in the project.

---

## Script Compilation

The CLI build compiles C# scripts via the Mono runtime rather than copying a pre-built `GameAssembly.dll`:

1. **Initialize Mono**: `MonoEntityManager::GetInstance().initialize()` — creates `MonoLoader` and `ScriptCompiler`
2. **Configure search paths**: `<project>/assets/scripts` is added as a script search directory
3. **Set output directory**: `GameAssembly.dll` is compiled directly to `<output>/data/managed/`
4. **Compile**: `MonoManager::StartCompilation()` collects `.cs` files and compiles them
5. **Validate**: Checks `compiler->LastCompileSucceeded()` and `compiler->diagnostics` for errors
6. **Copy engine DLLs**: `BasilEngine.dll` and `Engine.Bindings.dll` are copied alongside

### Error Handling

If compilation fails, all diagnostic messages are printed to `stderr`:

```
Scripts/Player.cs(42,10): Error CS0103: The name 'foo' does not exist in the current context
error: script compilation failed.
```

The build exits with code 1.

---

## Output Directory Structure

```
<output-dir>/<project-name>/
├── <project-name>.exe          # Game executable (renamed template)
├── config.yaml                 # Window title, resolution, fullscreen
├── fmod.dll                    # Audio runtime
├── mono-2.0-sgen.dll           # Mono runtime
├── resource.manifest           # Resource manifest (if applicable)
├── scene_manifest.order        # Scene ordering data
├── *.scene                     # Scene files
├── assets/
│   ├── bin/                    # Compiled resources (textures, meshes, materials)
│   │   ├── <guid>.texture
│   │   ├── <guid>.mesh
│   │   ├── <guid>.meshmeta
│   │   └── <guid>.material
│   └── audio/                  # Audio assets (non-.desc files)
│       └── *.wav
├── data/
│   └── managed/
│       ├── GameAssembly.dll    # Compiled C# scripts
│       ├── BasilEngine.dll     # Engine C# API
│       └── Engine.Bindings.dll # Engine bindings
└── lib/
    └── mono/                   # Mono runtime libraries
```

---

## GUI Build Mode

The GUI build flow (`BuildManager::BuildAsync`) has been updated to use the same explicit-path helpers:

- `MakeTemplateExecutable()` now accepts an explicit `projectDir` parameter
- `CopySceneManifestData()` now accepts an explicit `projectDir` parameter
- Material texture dependencies are resolved the same way

The GUI build continues to use `Engine::getWorkingDir()` for the project directory, passed explicitly to the refactored helpers. The async build uses `BuildContext` for progress reporting and supports abort via `BuildState::ABORTED`.

---

## Key Files

| File | Purpose |
|---|---|
| `editor/src/EntryPoint/main.cpp` | CLI argument parsing, branches to GUI or build mode |
| `editor/include/Manager/BuildManager.hpp` | BuildConfiguration, BuildContext, BuildSync/BuildAsync declarations |
| `editor/src/Manager/BuildManager.cpp` | Full build pipeline implementation (both sync and async) |
| `engine/include/Manager/MonoEntityManager.hpp` | Mono initialization and script compilation API |
| `lib/Scripting/include/MonoManager.hpp` | Mono runtime management singleton |
| `lib/Scripting/include/ScriptCompiler.hpp` | C# script compilation and diagnostics |
| `lib/resource_pipeline/ext/include/rsc-ext/registry.hpp` | ResourceTypeImporterRegistry for on-demand import |
| `lib/resource/ext/include/descriptors/material.hpp` | MaterialDescriptor with texture_properties |
| `lib/resource/core/include/native/material.h` | MaterialResourceData definition |
