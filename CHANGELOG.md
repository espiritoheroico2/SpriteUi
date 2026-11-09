# Changelog

Notable changes to SpriteUI are documented here. This project has not published a 0.6.0 release yet.

## Unreleased — 0.6.0

### Added

- C++20 layout engine with flex wrapping, content alignment and row baseline alignment.
- Text line breaking for a documented subset of Unicode opportunities, source-aware layout, caret rectangles and insertion hit testing.
- Nine-Slice calculation and rounded-panel SDF CPU reference.
- CTest smoke test that installs the library and builds a separate `find_package(SpriteUI)` consumer.
- Optional CPU microbenchmarks for layout and render extraction.
- GitHub Actions matrix for MSVC, GCC and Clang, plus a GCC ASan/UBSan job.
- Optional SDL3 (3.2+) mouse/focus input adapter, kept separate from graphics backends.
- Release-readiness, architecture status and consumer documentation.

### Still out of scope

- Diligent/GPU backend, Flecs adapter and native window/input integration.
- HarfBuzz/FreeType font shaping and a production MSDF atlas.
- Light Prism integration, platform validation beyond the documented local checks, and GPU parity testing.
