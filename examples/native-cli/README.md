# Native voice CLI consumer

This separately configured application combines iiVoiceOver, iiFileProvider and iiSDKCore. The former model backend remains retired; building this CLI does not restore synthesis.

```sh
cmake -S examples/native-cli -B build/native-cli \
  '-DCMAKE_PREFIX_PATH=/path/to/iiVoiceOver;/path/to/iiFileProvider;/path/to/iiSDKCore'
cmake --build build/native-cli
```
