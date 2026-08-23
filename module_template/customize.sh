SKIPUNZIP=0

ui_print "***************************************"
ui_print " RIWUTZ Per-App Debug Mask LAB v1.0"
ui_print "***************************************"
ui_print "- Requires NeoZygisk/Zygisk-compatible loader"
ui_print "- Target LAB: com.riwutz.debugmasklab"
ui_print "- ADB/adbd is NOT disabled globally"
ui_print "- No resetprop/system.prop is used"
ui_print "- Reboot after installation"

if [ ! -f "$MODPATH/zygisk/arm64-v8a.so" ] && [ ! -f "$MODPATH/zygisk/armeabi-v7a.so" ]; then
  abort "! Zygisk library missing. Build the source before flashing."
fi

set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/action.sh" 0 0 0755
