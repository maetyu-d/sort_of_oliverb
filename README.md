# MiVerb JUCE Plugin

This repository wraps the Mutable Instruments Parasites/Clouds reverb topology in a JUCE plugin (`MiVerb`).

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

The plugin target builds `VST3`, `AU`, and `Standalone` formats.

## Notes

- DSP is based on `parasites-master/clouds/dsp/fx/reverb.h` and adapted to run as a self-contained JUCE module.
- The original `stmlib` submodule is missing in this checkout, so minimal required DSP helpers are embedded in `Source/CloudsFxEngine.h`.
