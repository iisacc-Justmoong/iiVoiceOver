# iiVoiceOver

[Windows 빌드 및 실행](docs/WindowsBuildAndRun.md)

**0.2.0** provides native, local neural text-to-speech in C++23. It replaces the
old Qt string placeholder with voice-model, synthesis and audio-result objects.
The included English voice says **"Hello World"** using Piper, eSpeak NG and
ONNX Runtime CPU inference. Qt, Python, a virtual machine and a network service
are not used during synthesis. Initial configuration downloads pinned assets;
an installed package runs offline.

## Public API

```cpp
#include <iiVoiceOver.h>
#include <iostream>

int main() {
    auto voice = iiVoiceOver::VoiceModel::bundled();
    if (!voice) { std::cerr << voice.error().message; return 1; }
    iiVoiceOver::SpeechSynthesizer tts(std::move(*voice));
    auto audio = tts.synthesize("Hello World");
    if (!audio) { std::cerr << audio.error().message; return 1; }
    auto saved = audio->saveWave("hello-world.wav");
    if (!saved) { std::cerr << saved.error().message; return 1; }
}
```

| Object | Contract |
| --- | --- |
| `ModelFiles` | ONNX model, JSON configuration and native eSpeak data paths |
| `VoiceModel` | Immutable, copyable model description; validates paths/configuration and exposes language, speaker count and sample rate |
| `SpeechOptions` | Speed (0.25–4), zero-based speaker ID and retained sample limit |
| `SpeechSynthesizer` | Blocking native inference; returns `ii::core::Result<SpeechAudio>` and accepts `std::stop_token` |
| `SpeechAudio` | Owned, interleaved float32 PCM, sample rate, channels, duration and provider-backed WAV publication |

The bundled voice is `en_US-ljspeech-medium`, a US English female single-speaker
voice at 22,050 Hz. Generated PCM is mono. `saveWave()` writes a standard
little-endian PCM16 WAV through **iiFileProvider Native** and atomically creates
the destination. Existing files are never overwritten. Parent directories must
exist. WAV serialization also accepts validated stereo PCM supplied by callers.

`VoiceModel::load({model, config, espeak_data})` selects another local Piper
eSpeak-phonemized model; an empty configuration path means `model + ".json"`.
Model loading validates resources and configuration. ONNX graph loading occurs
inside `synthesize()`, so an invalid ONNX graph returns a synthesis error.
Other phonemizer types currently return `unsupported`. Voice licenses must be
checked individually; the bundled voice's terms do not cover other voices.

Text must contain 1–4096 UTF-8 bytes and no embedded NUL. Invalid parameters,
missing resources, failed inference, cancellation and existing output paths
return structured iiSDKCore errors. Nonfinite samples are rejected; inference
peaks are clipped to [-1, 1]. The sample limit bounds retained PCM, not the
internal ONNX allocator or the size of a sentence already being inferred.

The native eSpeak backend has process-global state. iiVoiceOver serializes the
whole native session lifetime across its synthesizers, loading and releasing a
native voice session per request. This avoids one synthesizer terminating the
phonemizer used by another. Calls on the same or different objects are safe;
they are not parallel inference. Other libraries independently calling eSpeak
in the same process must coordinate their usage. Run `synthesize()` on a worker
and post its owned result through the application's iiSDKCore execution context.

Cancellation is observed before initialization, while waiting for the backend,
and between sentence chunks. Native model initialization and an ONNX inference
already in progress finish before cancellation is returned. Cancelled requests
return no partial PCM and create no file. This API does not promise real-time
streaming or immediate interruption of a running native kernel.

`helloWorld()` now returns `std::string("Hello World")`. The former `QString`
placeholder API is intentionally replaced. Package compatibility is restricted
to the same minor version and the native library has SOVERSION 0.2.

## Build, test and install

Requirements: CMake 3.31+, a C++23 compiler, native C/C++ build tools and Git;
installed Qt-free iiSDKCore 0.18+ and iiFileProviderNative 0.8+. The default SDK
prefixes are discovered under `$HOME/.local/SDK/`. Supply `CMAKE_PREFIX_PATH`
for custom dependency installations.

On Windows, the native Piper DLL and its MSVC import library are both declared
as external build outputs. The Ninja build-graph regression checks that linking
the SDK can produce `lib/piper.lib` from a clean build without a manual first pass.

```sh
./install.sh
```

The script configures Release in the fixed `build/`, builds native dependencies
and the SDK, runs CTest, installs to `$HOME/.local/SDK/iiVoiceOver`, and builds
and runs an independent installed consumer in `build/consumer/build/`.
Qt package discovery is explicitly disabled during these checks.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE
cmake --build build --parallel 8
ctest --test-dir build --output-on-failure
cmake --install build
./build/bin/ii-voice-over --text "Hello World" --output build/hello-world.wav --play
```

The example's synthesis and WAV writing use iiVoiceOver. `--play` starts the
native WAV player through **iiSDKCore Process** (macOS `afplay`, Linux `aplay`).
The OS player only plays the generated waveform; it does not synthesize speech.
On Windows the example saves the WAV without a playback helper. The TTS library
does not depend on the example's process helper or on an audio-device library;
applications can submit float32 samples to iiDeviceMultimedia themselves.
Each example output must be a fresh filename, because publication is exclusive.

The build pins libpiper and its native phonemizers by commit, downloads ONNX
Runtime 1.22.0 and the 63 MB voice with SHA-256 verification, and keeps all
dependencies and generated files under `build/`. The model's manifest and
original card are tracked under `models/`; model binaries are reproducibly
downloaded rather than checked into Git. Default native ONNX distributions
are selected for macOS arm64/x86_64, Linux aarch64/x86_64 and Windows x64.
For another native SDK distribution set `IIVOICEOVER_ORT_ROOT` to a directory
containing `include/` and `lib/`; there is no emulated fallback. Cross-compiling
the third-party phonemizers is not configured by this initial build integration.
Runtime execution evidence for this change is **macOS arm64** only.

Options: `IIVOICEOVER_DOWNLOAD_VOICE=OFF` omits the demo voice; applications must
load their own files. `IIVOICEOVER_BUILD_EXAMPLES=OFF` omits the CLI. The default
test configuration downloads the real voice and exercises API/file contracts,
single/multiple sentence inference, speed changes, concurrent requests and
recovery after a corrupt ONNX model. Tests check nonempty/non-silent finite PCM
and the WAV structure; they do not independently transcribe the generated audio.

## Installed consumers

```cmake
find_package(iiVoiceOver 0.2 CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE iiVoiceOver::iiVoiceOver)
```

The target exports C++23 and iiSDKCore Contracts. iiFileProvider Native is a
private native runtime dependency. The package installs `iiVoiceOver.h`, its
shared library, libpiper, ONNX Runtime, eSpeak data, the English model/config/card,
CLI, documentation, CMake targets and third-party notices. Bundled resources
are resolved relative to the loaded iiVoiceOver library, allowing the package
to be moved together with its `lib/`, `bin/` and `share/` layout. iiSDKCore and
iiFileProvider remain separately installed SDK dependencies.

## License and source layout

SPDX-License-Identifier: **MIT**. Self-written code and documentation
use [LICENSE](LICENSE). See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for
Piper GPLv3, eSpeak GPLv3+, Open JTalk/MeCab, ONNX Runtime MIT, header notices and
voice provenance. The upstream voice card states that it was trained from
scratch on public-domain LJ Speech data. All notices ship with the package.

Headers and implementations live together under `src/`. Examples, tests, model
manifests and CMake integration have separate directories. All build output
stays within `build/`.
