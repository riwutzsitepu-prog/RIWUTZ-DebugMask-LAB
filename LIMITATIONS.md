# Limitations

1. **SystemProperties only**
   This module covers Android `SystemProperties` reads routed through the hooked JNI methods.
   It does not intercept arbitrary file reads, Binder calls, `/proc`, USB device APIs,
   Settings Provider calls, native property APIs used directly by third-party native code,
   or application-specific detection logic.

2. **Settings.Global is not spoofed**
   `adb_enabled` and `development_settings_enabled` are Settings Provider values.
   The diagnostic action prints the real values so you can see that global ADB was not altered.

3. **Android-version variation**
   Android 10 exposes the five key-based JNI methods used here. Other Android releases may
   move or wrap some methods differently. The module logs how many compatible hooks were found.

4. **Target is compile-time**
   The default target is `com.riwutz.debugmasklab`. This avoids a runtime arbitrary-target
   mechanism. For legitimate development testing, change `TARGET_PACKAGE` only to an app you
   own/control and rebuild.
