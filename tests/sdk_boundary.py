from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
files = [root / "CMakeLists.txt", root / "cmake/iiVoiceOverConfig.cmake.in",
         root / "src/iiVoiceOver.h", root / "src/iiVoiceOver.cpp"]
for path in files:
    content = path.read_text()
    for forbidden in ("iiSDKCore", "iiFileProvider"):
        if forbidden in content:
            raise SystemExit(f"SDK dependency {forbidden} remains in {path}")
print("iiVoiceOver SDK boundary passed")
