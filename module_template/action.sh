#!/system/bin/sh

OUT="/sdcard/RIWUTZ_DebugMask_DIAG.txt"
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
} > "$OUT"

echo "Saved: $OUT"
cat "$OUT"
