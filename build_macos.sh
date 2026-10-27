#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
SDK="$(xcrun --sdk iphoneos --show-sdk-path)"
mkdir -p out
xcrun --sdk iphoneos clang -arch arm64 -miphoneos-version-min=15.0 -isysroot "$SDK" \
  -O2 -fPIC -dynamiclib -Wall -Wextra \
  -Wl,-install_name,@loader_path/.jbroot/Library/MobileSubstrate/DynamicLibraries/PGRealAimESP.dylib \
  -framework UIKit -framework Foundation -framework CoreFoundation -framework QuartzCore -framework AVFAudio -lobjc \
  -o out/PGRealAimESP.dylib src/PG_Neon_v040.c
codesign --force --sign - out/PGRealAimESP.dylib
codesign --verify out/PGRealAimESP.dylib
python3 make_deb.py out/PGRealAimESP.dylib out/PG_Neon_v040_RootHide_EXPERIMENTAL.deb
file out/PGRealAimESP.dylib
