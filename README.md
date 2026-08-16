# RainbowChams — GLES2 chams + memory toolkit (arm64, rootless)

Educational / authorized-security laboratory project.
Native GL chams for Android GLES2 games, no root required.

## Structure
- plt_hook.cpp — PLT/GOT hook engine (dl_iterate_phdr + DT_RELA/JMPREL rewrite)
- gl_hook.cpp  — glDrawElements/glDrawArrays hooks: depth GL_ALWAYS,
                 depth-write off, cull off, animated HSV rainbow tint
- memscan.cpp  — in-process value scanner over /proc/self/maps RW regions
- entry.cpp    — constructor + JNI_OnLoad entry, retry loop until GL lib loads

## Build (Android NDK only — no other tooling)
    export ANDROID_NDK_HOME=/path/to/ndk
    $ANDROID_NDK_HOME/ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk
    # -> libs/arm64-v8a/librainbow.so

## Load (rootless)
- Repack the target APK: add lib/arm64-v8a/librainbow.so
- In the launcher activity's onCreate (smali), after invoke-super, insert:
      const-string v0, "rainbow"
      invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
  and raise .locals by 1 so v0 is safe.
- Re-sign and install. The library self-starts via the constructor.

## Config
Push rc.conf to /sdcard/RC/rc.conf. mode: 0=off, 1=chams, 2=chams+rainbow.

## Target-specific chams (the RE step)
The generic hook tints whatever shader is active. To isolate players:
1. In hk_glDrawElements, log GL_CURRENT_PROGRAM + bound VAO + count/mode.
2. Correlate ids with player spawn/kill over a match.
3. Allowlist the player program id inside apply_rainbow_tint() (TODO marker).

## Notes
- Re-signing breaks Play Integrity/SafetyNet; server-side tamper checks
  will reject modified clients — lab/emulator use only.
- No ptrace, no root: pure GOT redirection (the standard commercial
  Android game-cheat technique; useful for studying anti-cheat).
## Dear ImGui integration

`jni/imgui/` contains the Dear ImGui core plus the Android/OpenGL ES backend
from the supplied `imgui-master.zip`.

`jni/imgui_overlay.cpp` provides:
- a small floating `RC` ImGui button
- tap to open/close a dark RainbowChams panel
- Enabled and Mode controls
- optional ImGui demo window

The UI is exposed through `rc_imgui_init()`, `rc_imgui_frame()`, and
`rc_imgui_shutdown()`. These functions must be called from the Android app's
own render loop while an EGL/GLES context is current. The supplied native
library is not an Android Activity and does not itself own an `ANativeWindow`,
so this archive does not fabricate a renderer hook or window handle.


## License key UI

The ImGui overlay is now gated by a license screen. A valid key unlocks the
floating button and the main panel. Valid durations are 1 day, 1 week, 1 month,
and 1 year.

### Generate keys (private/admin side)

From the `tools` directory:

    python3 keygen.py 1d
    python3 keygen.py 1w
    python3 keygen.py 1m
    python3 keygen.py 1y

The Android client stores an activated key at `/sdcard/RC/license.key`.

**Important:** this implementation is a local/demo license gate. The key embeds
its expiry timestamp, so it is not suitable for a real paid service by itself.
For production, issue and verify signed licenses on a server and keep only a
public verification key in the app.

## Paid licensing hardening
The `server/` directory contains a server-verified licensing backend. Use it for real paid keys rather than relying on the old timestamp-only client validation. See `server/README.md` and `HARDENING.md`.
