# Voice WAV file composition

This separately configured consumer encodes PCM with iiVoiceOver and uses iiFileProvider for exclusive publication. The test creates only temporary resources, verifies existing files are preserved, and removes its outputs.

```sh
cmake -S examples/file-provider -B build/file-consumer \
  '-DCMAKE_PREFIX_PATH=/path/to/iiVoiceOver;/path/to/iiFileProvider'
cmake --build build/file-consumer
ctest --test-dir build/file-consumer --output-on-failure
```
