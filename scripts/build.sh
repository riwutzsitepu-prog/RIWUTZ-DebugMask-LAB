#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODULE="$ROOT/module"
TEMPLATE="$ROOT/module_template"
DIST="$ROOT/dist"
STAGE="$DIST/stage"
OUT="$DIST/RIWUTZ_PerApp_DebugMask_LAB_v1.1.zip"

"$ROOT/scripts/fetch_zygisk_header.sh"

if command -v ndk-build >/dev/null 2>&1; then
  NDK_BUILD="$(command -v ndk-build)"
elif [[ -n "${ANDROID_NDK_HOME:-}" && -x "$ANDROID_NDK_HOME/ndk-build" ]]; then
  NDK_BUILD="$ANDROID_NDK_HOME/ndk-build"
elif [[ -n "${ANDROID_NDK_ROOT:-}" && -x "$ANDROID_NDK_ROOT/ndk-build" ]]; then
  NDK_BUILD="$ANDROID_NDK_ROOT/ndk-build"
else
  echo "Android NDK not found. Install NDK r21+ and expose ndk-build or ANDROID_NDK_HOME." >&2
  exit 2
fi

rm -rf "$MODULE/libs" "$MODULE/obj" "$STAGE"
mkdir -p "$DIST" "$STAGE/zygisk"

(
  cd "$MODULE"
  "$NDK_BUILD" \
    NDK_PROJECT_PATH=. \
    NDK_APPLICATION_MK=jni/Application.mk \
    APP_BUILD_SCRIPT=jni/Android.mk \
    -j"${JOBS:-2}"
)

for ABI in arm64-v8a armeabi-v7a; do
  SRC="$MODULE/libs/$ABI/libriwutz_debugmask.so"
  if [[ ! -f "$SRC" ]]; then
    echo "Missing build output: $SRC" >&2
    exit 3
  fi
done

cp -a "$TEMPLATE/." "$STAGE/"
cp "$MODULE/libs/arm64-v8a/libriwutz_debugmask.so" "$STAGE/zygisk/arm64-v8a.so"
cp "$MODULE/libs/armeabi-v7a/libriwutz_debugmask.so" "$STAGE/zygisk/armeabi-v7a.so"
chmod 0755 "$STAGE/action.sh" "$STAGE/customize.sh"

rm -f "$OUT"
(
  cd "$STAGE"
  zip -qr9 "$OUT" .
)

rm -rf "$STAGE"
echo "Built: $OUT"
