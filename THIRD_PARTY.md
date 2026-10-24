# Dependencies

JUCE 8.0.12: https://github.com/juce-framework/JUCE/tree/8.0.12 . JUCE includes its VST3 wrapper, SDK headers, and platform libraries used by this project. Its licensing entry point is `vendor/JUCE/LICENSE.md`; module-specific licensing and third-party notices are retained in the checkout. The package script copies the top-level JUCE licence alongside the binaries when using the local vendor checkout.

Build tools: Microsoft Visual Studio C++ Build Tools and CMake. These are required to rebuild but are not included in the package.

The FL Studio installation is a reference target for investigation only. Its binaries and resources are not dependencies and are not included in the generated plugin or standalone application.
