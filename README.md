# RIWUTZ Per-App Debug Mask LAB v1.0

Build-ready Zygisk module source for Android compatibility testing in an app you own/control.
The default process target is intentionally a lab package:

`com.riwutz.debugmasklab`

## What it does

Inside the target process only, calls through Android 10 `android.os.SystemProperties` are masked for these keys:

- `sys.usb.config` -> `mtp`
- `persist.sys.usb.config` -> `mtp`
- `sys.usb.state` -> `mtp`
- `init.svc.adbd` -> `stopped`
- `service.adb.tcp.port` -> `-1`

All other app processes unload the module library immediately.

## What it deliberately does NOT do

It does not call `stop adbd`, `setprop`, `resetprop`, or `settings put`.
Therefore ADB remains enabled globally and host-side ADB/mirroring can continue to work.

It also does **not** hook `Settings.Global.ADB_ENABLED` or
`Settings.Global.DEVELOPMENT_SETTINGS_ENABLED`. Those values are served via the
Settings Provider / Java APIs, not the SystemProperties JNI methods hooked here.
A pure small Zygisk JNI module should not claim those values are masked when they are not.

## Build requirements

- Android NDK r21 or newer
- `ndk-build`
- `curl`
- `zip`

Run:

```bash
./scripts/build.sh
```

Output:

`dist/RIWUTZ_PerApp_DebugMask_LAB_v1.0.zip`

The build script automatically downloads the canonical public `zygisk.hpp` from
`topjohnwu/zygisk-module-sample` if it is not already present.

## GitHub Actions build

This source includes `.github/workflows/build.yml`. Push the project to a repository,
open **Actions -> Build RIWUTZ DebugMask LAB -> Run workflow**, then download the
artifact produced by the workflow.

## Install / test

1. Enable NeoZygisk or another Zygisk-compatible loader.
2. Build the ZIP.
3. Install the resulting ZIP using the compatible root module manager.
4. Reboot.
5. Start your lab app (`com.riwutz.debugmasklab`).
6. Use the module's Action button, or run `action.sh`, to create
   `/sdcard/RIWUTZ_DebugMask_DIAG.txt`.
7. Check logcat tag `RiwutzPerAppDebugMask` for hook activity.

## Expected global state

ADB stays real at system level. For example a rooted shell may still report:

```text
adb_enabled=1
init.svc.adbd=running
```

That is expected. The mask exists only inside the configured lab process.
