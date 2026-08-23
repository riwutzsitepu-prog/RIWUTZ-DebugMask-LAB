#!/system/bin/sh

NAME="RIWUTZ_DebugMask_DIAG.txt"
TMP="/data/local/tmp/$NAME"
TAG="RiwutzPerAppDebugMask"
TARGET="com.riwutz.debugmasklab"

{
  echo "RIWUTZ Per-App Debug Mask LAB - diagnostic"
  echo "Date: $(date)"
  echo "Target: $TARGET"
  echo
  echo "=== DEVICE ==="
  echo "Android: $(getprop ro.build.version.release)"
  echo "SDK: $(getprop ro.build.version.sdk)"
  echo "ABI64: $(getprop ro.product.cpu.abilist64)"
  echo "ABI32: $(getprop ro.product.cpu.abilist32)"
  echo
  echo "=== STORAGE PATHS ==="
  ls -ld /sdcard /storage/emulated/0 /data/media/0 2>&1
  echo
  echo "=== GLOBAL ADB STATE (must stay real) ==="
  echo "adb_enabled=$(settings get global adb_enabled 2>/dev/null)"
  echo "development_settings_enabled=$(settings get global development_settings_enabled 2>/dev/null)"
  echo "init.svc.adbd=$(getprop init.svc.adbd)"
  echo "sys.usb.config=$(getprop sys.usb.config)"
  echo "persist.sys.usb.config=$(getprop persist.sys.usb.config)"
  echo "sys.usb.state=$(getprop sys.usb.state)"
  echo "service.adb.tcp.port=$(getprop service.adb.tcp.port)"
  echo
  echo "=== TARGET PROCESS ==="
  pidof "$TARGET" 2>/dev/null || echo "not running"
  echo
  echo "=== MODULE LOG ==="
  logcat -d -v time -s "$TAG:D" '*:S' 2>/dev/null | tail -n 250
  echo
  echo "NOTE: Settings.Global adb_enabled/development_settings_enabled are diagnostics only."
  echo "This lab module masks Android SystemProperties only and does not hook Settings Provider."
} > "$TMP" 2>&1

chmod 0644 "$TMP" 2>/dev/null
SAVED=""

# Android 10 root managers can have a different /sdcard mount namespace.
# Writing through /data/media/0 is the most reliable way to expose a file to user 0.
if [ -d /data/media/0 ] && cp -f "$TMP" "/data/media/0/$NAME" 2>/dev/null; then
  chown 1023:1023 "/data/media/0/$NAME" 2>/dev/null
  chmod 0664 "/data/media/0/$NAME" 2>/dev/null
  command -v restorecon >/dev/null 2>&1 && restorecon "/data/media/0/$NAME" 2>/dev/null
  SAVED="/storage/emulated/0/$NAME"
fi

# Also try the normal emulated-storage paths when visible in this namespace.
for DEST in "/storage/emulated/0/$NAME" "/sdcard/$NAME"; do
  if [ -d "$(dirname "$DEST")" ] && cp -f "$TMP" "$DEST" 2>/dev/null; then
    chmod 0664 "$DEST" 2>/dev/null
    SAVED="$DEST"
    break
  fi
done

echo ""
if [ -n "$SAVED" ]; then
  echo "Saved: $SAVED"
else
  echo "Storage copy failed; fallback is always available at: $TMP"
fi
echo "Fallback: $TMP"
echo ""
cat "$TMP"
