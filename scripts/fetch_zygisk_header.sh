#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DST="$ROOT/module/jni/zygisk.hpp"
URL="https://raw.githubusercontent.com/topjohnwu/zygisk-module-sample/master/module/jni/zygisk.hpp"

if [[ -s "$DST" ]]; then
  echo "zygisk.hpp already present: $DST"
  exit 0
fi

command -v curl >/dev/null 2>&1 || { echo "curl is required" >&2; exit 1; }
curl -fL --retry 3 "$URL" -o "$DST"
echo "Fetched canonical Zygisk header -> $DST"
