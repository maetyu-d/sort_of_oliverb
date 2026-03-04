# Sort of Oliverb JUCE Plugin

![](https://github.com/maetyu-d/sort_of_oliverb/blob/main/Screenshot%202026-03-04%20at%2021.27.49.png)

This repository wraps the Mutable Instruments Parasites/Clouds reverb topology in a JUCE plugin (`OliVerb`) and adds some extras.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

The plugin target builds `VST3`, `AU`, and `Standalone` formats.

## Notes

- DSP is based on `parasites-master/clouds/dsp/fx/reverb.h` and adapted to run as a self-contained JUCE module.
- The original `stmlib` submodule is missing in this checkout, so minimal required DSP helpers are embedded in `Source/CloudsFxEngine.h`.
