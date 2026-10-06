# iiVoiceOver

Version 0.1.0 dynamic placeholder SDK using C++20 and Qt 6.8.3 Core. Public functions return only `Hello world!` strings, and product features corresponding to SDK names are not yet implemented. No external dependencies other than Qt are added.

<a id="공개-api"></a>

## Public API

```cpp
#include <iiVoiceOver.h>

const QString message = iiVoiceOver::helloWorld(); // Exactly "Hello world!"
```

Public headers and implementation are placed together at `src/iiVoiceOver.h`, `src/iiVoiceOver.cpp` in the repository root. `IIVOICEOVER_EXPORT` exports symbols, and `IIVOICEOVER_BUILDING_LIBRARY` is defined only for library builds.

<a id="빌드-테스트-설치"></a>

## Build, test, install

CMake 3.24 and above, C++20 compiler, and Qt **6.8.3** Core development package are required. Qt version must match exactly in CMake. If `/Volumes/Storage/Qt/6.8.3/macos` is present at macOS, the installation script is used automatically.

```sh
./install.sh
```

The script configures and builds Release and runs CTest in the fixed `build/`, then installs to `$HOME/.local/SDK/iiVoiceOver`. It next configures `tests/consumer` independently in `build/consumer/build/`, building and running using only the installed CMake package, headers, and dynamic library. Tests verify the returned strings, whether C++20 is used, and the Qt compile and runtime versions 6.8.3.

The path is specified as an environment variable, and the script does not accept separate command-line arguments. When passing multiple paths to `CMAKE_PREFIX_PATH`, they are separated by semicolons.

```sh
QT_PREFIX_PATH="/Volumes/Storage/Qt/6.8.3/macos" \
INSTALL_PREFIX="$HOME/.local/SDK/iiVoiceOver" \
CMAKE_PREFIX_PATH="/additional/cmake/prefix" ./install.sh
```

Manual build and verification also use the same `build/`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/Volumes/Storage/Qt/6.8.3/macos" -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build
```

<a id="설치-결과와-소비자-연결"></a>

## Installation results and consumer connections

Under the default installation path, `include/iiVoiceOver.h`, `lib/` shared libraries, `lib/cmake/iiVoiceOver/` Config·ConfigVersion·Targets, and `share/iiVoiceOver/README.md` are created. Windows runtime DLLs are installed to `bin/`. The default installation path applies only when this SDK is the top-level CMake project. When configuring directly with CMake, if HOME is missing, USERPROFILE is used as the default installation path.

```cmake
find_package(iiVoiceOver 0.1.0 CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE iiVoiceOver::iiVoiceOver)
```

Consumers specify the `CMAKE_PREFIX_PATH` SDK installation path and Qt 6.8.3 path together. The exported target passes C++20 requirements and `Qt6::Core` connections. The installation library sets `INSTALL_RPATH_USE_LINK_PATH` to preserve the runtime search path of the connected external library. Qt uses a separately installed runtime and does not copy and distribute it. Qt usage and redistribution apply the license conditions of the corresponding Qt installation.

## License

SPDX-License-Identifier: AGPL-3.0-only

Self-written code and documents of iiVoiceOver are distributed exclusively under the GNU Affero General Public License v3.0. The full terms follow [LICENSE](LICENSE).

External libraries including Qt and third-party code with separate notices maintain their own licenses. This project's license declaration does not replace the corresponding third-party license.

## Source layout

Implementation files and their headers live together under `src/`. Existing feature and platform subdirectories retain their responsibilities. Build configuration, tests, documentation, resources, and maintenance scripts remain at the project root. Configure and build using the repository-local `build/` directory.
