# iiVoiceOver

A C++23 PCM audio API with explicit availability reporting for speech synthesis.
The previous Piper/ONNX learned voice backend and its bundled voice are retired.
`SpeechSynthesizer::available()` returns false; `VoiceModel::load()` and `bundled()`
report unsupported model inference. No model or native inference runtime is
automatically downloaded.

`SpeechAudio` retains owned float32 PCM, duration calculation, and validated
PCM16 WAV encoding through `waveBytes()`. The independent SDK does not link
other SDKs. An application can compose it with iiFileProvider using the
separately configured [file consumer](examples/file-provider), which publishes
exclusively and never overwrites an existing destination. PCM encoding does not
require a learned model.

`Result` and `ErrorCode` now belong to `iiVoiceOver` rather than iiSDKCore.
The `SpeechAudio::saveWave(path)` member was removed: migrate to the consumer
adapter or publish `waveBytes()` in the application, and rebuild callers.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

The [native CLI](examples/native-cli) is a separate consumer of iiVoiceOver,
iiSDKCore and iiFileProvider. It reports `synthesisAvailable` and
`pcmExportAvailable`. A speech request fails explicitly until a separate
supported native backend is implemented.

See [the runtime retirement contract](docs/RuntimeRetirement.md) and the public
API in `src/iiVoiceOver.h`. Historical Piper licensing records do not describe a
runtime included in current builds.

<!-- workspace-docs:readme:start -->
## Documentation entry points

- Human overview: this English `README.md`.
- Agent instructions: [AGENT.md](AGENT.md).
- Human documentation: [documentation index](docs/README.md).
- Structured agent documentation: [agent index](docs/agent-index.json).
<!-- workspace-docs:readme:end -->
